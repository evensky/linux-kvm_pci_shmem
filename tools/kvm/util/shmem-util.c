#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "shmem-util.h"

inline void *setup_shmem(const char *key, size_t len, int creating, int verbose)
{
	int fd;
	int rtn;
	void *mem;
	int flag = O_RDWR;
	if (creating)
		flag |= O_CREAT;
	fd = shm_open(key, flag, S_IRUSR | S_IWUSR);
	if (fd == -1) {
		fprintf(stderr, "Failed to open shared memory file %s\n", key);
		fflush(stderr);
		return NULL;
	}
	if (creating) {
	  if (verbose)
	    fprintf(stderr,"Truncating file.\n");
		rtn = ftruncate(fd, (off_t) len);
		if (rtn == -1) {
			fprintf(stderr, "Can't ftruncate(fd,%ld)\n", len);
			fflush(stderr);
		}
	}
	mem = mmap(NULL, len,
		   PROT_READ | PROT_WRITE, MAP_SHARED | MAP_NORESERVE, fd, 0);
	close(fd);
	if (mem == NULL) {
		fprintf(stderr, "Failed to mmap shared memory file\n");
		fflush(stderr);
		return NULL;
	}
	if (creating) {
	  int fill_bytes = 0xae0dae0d;
	  if (verbose)
	    fprintf(stderr,"Filling buffer\n");
	  //fill_mem(mem,len,(char *)&fill_bytes,4);
	  fill_mem(mem,len,(char *)&fill_bytes,1);
	}
	return mem;
}

inline void fill_mem(void *buf, size_t buf_size, char *fill, size_t fill_len)
{
	size_t i;

	if (fill_len == 1) {
		memset(buf, fill[0], buf_size);
	} else {
		if (buf_size > fill_len) {
			for (i = 0; i < buf_size - fill_len; i += fill_len)
			  memcpy(((char *)buf) + i, fill, fill_len);
			memcpy(buf + i, fill, buf_size - i);
		} else {
			memcpy(buf, fill, buf_size);
		}
	}
}
