#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <asm/types.h>
#include <unistd.h>
#include <ctype.h>
#include "shmem-util.h"
#include "kvm/pci-shmem.h"

void usage(const char *s, int terminate);
static struct shmem_info *shmem_parser(const char *arg);

static struct shmem_info *shmem_parser(const char *arg)
{
	const uint64_t default_size = 16 * 1024 * 1024;
	const uint64_t default_phys_addr = 0xc8000000;
	const char *default_handle = "/kvm_shmem";
	enum { PCI, UNK } addr_type = PCI;
	uint64_t phys_addr;
	uint64_t size;
	char *handle = NULL;
	int create = 0;
	const char *p = arg;
	char *next;
	int base = 10;

	printf("shmem_parser(%s)\n", arg);
	if (strcasestr(p, "pci:")) {
		p += 4;
		addr_type = PCI;
	} else if (strcasestr(p, "mem:")) {
		fprintf(stderr, "I can't add to E820 map yet.\n");
		exit(1);
	}
	base = 10;
	if (strcasestr(p, "0x"))
		base = 16;
	phys_addr = strtoll(p, &next, base);
	if (next == p && phys_addr == 0) {
		fprintf(stderr,
			"shmem: no physical addr specified, using default.\n");
		phys_addr = default_phys_addr;
	}
	if (*next != ':' && *next != '\0') {
		fprintf(stderr, "shmem: unexpected chars after phys addr.\n");
		exit(1);
	}
	if (*next == '\0')
		p = next;
	else
		p = next + 1;
	base = 10;
	if (strcasestr(p, "0x"))
		base = 16;
	size = strtoll(p, &next, base);
	if (next == p && size == 0) {
		fprintf(stderr, "shmem: no size specified, using default.\n");
		size = default_size;
	}
	if (*next != ':' && *next != '\0') {
		fprintf(stderr,
			"shmem: unexpected chars after phys size. <%c><%c>\n",
			*next, *p);
		exit(1);
	}
	if (*next == '\0')
		p = next;
	else
		p = next + 1;
	if (*p && (next = strcasestr(p, "handle="))) {
		if (p != next) {
			fprintf(stderr, "unexpected chars before handle\n");
			exit(1);
		}
		p += 7;		// skip handle=
		next = strchrnul(p, ':');
		if (next - p) {
			handle = malloc(next - p + 1);
			strncpy(handle, p, next - p);
			handle[next - p] = '\0';	// just in case.
		}
		if (*next == '\0')
			p = next;
		else
			p = next + 1;
	}
	if (*p && strcasestr(p, "create")) {
		create = 1;
		p += strlen("create");
	}
	if (*p != '\0') {
		fprintf(stderr, "shmem: unexpected trailing chars\n");
		exit(1);
	}
	if (handle == NULL) {
		handle = malloc(strlen(default_handle) + 1);
		strcpy(handle, default_handle);
	}
	printf("shmem: phys_addr = %lx\n", phys_addr);
	printf("shmem: size      = %lx\n", size);
	printf("shmem: handle    = %s\n", handle);
	printf("shmem: create    = %d\n", create);
	struct shmem_info *si = malloc(sizeof(struct shmem_info));
	si->phys_addr = phys_addr;
	si->size = size;
	si->handle = handle;
	si->create = create;
	return si;
}

void usage(const char *s, int terminate)
{
	printf("Usage: %s -D <shmem args> -d -v -w -s <fillstring> -f <file>\n",
	       s);
	printf
	    ("\t shmem args := [pci:]<addr>:<size>[:handle=<shm handle>][:create]]\n");
	printf
	    ("\t\t addr : isn't used, but here for compatibility with kvm tool\n");
	printf("\t\t size : size of shared memory segment in bytes\n");
	printf("\t\t shm handle: string to use for shm_open()\n");
	printf
	    ("\t\t create : should this program create and initialize shared memory segment\n");
	printf("\t -d : bump debug level\n");
	printf("\t -v : bump verbose level\n");
	printf
	    ("\t -w : don't exit immediately. Used to keep shmem segments alive for others\n");
	printf("\t -s : fill memory region with specified bytes\n");
	printf("\t -f : fill memory region with contents of specified file\n");
	if (terminate)
		exit(terminate);
}

int main(int argc, char **argv)
{
	void *host_mem;
	int debug = 0;
	int verbose = 0;
	char *fillbytes = 0;
	int fill_len = 0;
	char *fname = 0;
	int wait = 0;
	int opt, sz;
	struct shmem_info *si = NULL;
	while ((opt = getopt(argc, argv, "D:dvws:f:")) != -1) {
		switch (opt) {
		case 'D':
			si = shmem_parser(optarg);
			if (si == NULL)
				usage(argv[0], 1);
			break;
		case 'd':
			debug++;
			break;
		case 'v':
			verbose++;
			break;
		case 'w':
			wait++;
			break;
		case 's':
			fill_len = strlen(optarg);
			if (strcasestr(optarg, "0x") && fill_len > 2) {
				int opt_len = fill_len;
				int i;
				fill_len = ((fill_len - 2) + 1) >> 1;
				fillbytes = malloc(fill_len);
				bzero(fillbytes, fill_len);
				printf("fill_len=%d opt_len=%d\n", fill_len,
				       opt_len);
				for (i = 2; i < opt_len; i++) {
					unsigned char c = 0;
					if (isxdigit(optarg[i])) {
						if (isdigit(optarg[i])) {
							c = optarg[i] - '0';
						} else if (isupper(optarg[i])) {
							c = optarg[i] - 'A' +
							    10;
						} else {
							c = optarg[i] - 'a' +
							    10;
						}
					} else {
						fprintf(stderr,
							"error parsing fill hex digits\n");
						fprintf(stderr,
							"i=%d (%c) (%x)\n", i,
							optarg[i], optarg[i]);
						usage(argv[0], 1);
					}
					int ibyte = (i - 2) / 2;
					printf("i=%d ibyte=%d c=%x\n", i, ibyte,
					       c);
					if (0x01 & i) {	// odd
						fillbytes[ibyte] |= (0x0f & c);
					} else {
						fillbytes[ibyte] |=
						    ((0x0f & c) << 4);
					}
				}
				for (i = 0; i < fill_len; i++) {
					printf("fillbyte[%d]=0x%02x\n", i,
					       (unsigned char)fillbytes[i]);
				}
			} else {
				fillbytes = malloc(fill_len + 1);
				strncpy(fillbytes, optarg, fill_len);
			}
			break;
		case 'f':
			sz = strlen(optarg);
			fname = malloc(sz + 1);
			strncpy(fname, optarg, sz);
			break;
		default:
			usage(argv[0], 1);
			break;
		}
	}

	size_t buf_size = si->size;
	const char *key = si->handle;

	if (verbose) {
		printf("Filling shmem key %s of %ldMB size\n", key,
		       buf_size / 1024 / 1024);
		printf("Filling with string <%*s> if size %d bytes\n", fill_len,
		       fillbytes, fill_len);
		printf("Filling from file <%s>\n", fname);
		printf("Wait flag set to %d\n", wait);
	}
#if 0
	if (debug)
		printf
		    ("Sizeof: size_t(%d),off_t(%d),__u64(%d) unsigned long long(%d)\n",
		     sizeof(size_t), sizeof(off_t), sizeof(__u64),
		     sizeof(unsigned long long));
#endif
	host_mem = setup_shmem(key, buf_size, si->create, verbose);
	if (host_mem == NULL) {
		fflush(stderr);
		exit(1);
	}

	if (fillbytes && fill_len)
		fill_mem(host_mem, buf_size, fillbytes, fill_len);

	struct stat statbuf;
	void *mem_fd = 0;

	if (fname) {
		int rt;
		int fd = open(fname, O_RDONLY);
		if (fd == -1) {
			printf("Failed to open file <%s>\n", fname);
			exit(1);
		}
		rt = fstat(fd, &statbuf);
		if ((unsigned long long)statbuf.st_size >
		    (unsigned long long)buf_size) {
			printf
			    ("File, %s is too large. We allocated %ld MB but the file is %ld MB.\n",
			     fname, buf_size / 1024 / 1024,
			     statbuf.st_size / 1024 / 1024);
			exit(1);
		}
		mem_fd =
		    mmap(0, statbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
		if (mem_fd == NULL) {
			printf("Failed to mmap file %s.\n", fname);
			exit(1);
		}
		if (verbose)
			printf("Filling memory now.\n");
		//fill_mem(host_mem,buf_size,mem_fd,statbuf.st_size);
		memcpy(host_mem, mem_fd, statbuf.st_size);
		if (verbose)
			printf("Back from filling memory.\n");
		munmap(mem_fd, statbuf.st_size);
	}
	if (wait > 0) {
		fgetc(stdin);
	}

	fflush(stdout);
	fflush(stderr);
	exit(0);
}
