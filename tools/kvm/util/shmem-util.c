#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "shmem-util.h"

inline void * dae_shmem(const char *key,size_t len,int creating,
			       int verbose) {
  int fd;
  int rtn;
  void *mem;
  int flag = O_RDWR;
  if (creating)
    flag |= O_CREAT;
  fd = shm_open(key,flag,S_IRUSR|S_IWUSR);
  if (fd == -1) {
    fprintf(stderr,"Failed to open shared memory file %s\n",key);
    fflush(stderr);
    return NULL;
  }
  if (creating) {
    rtn = ftruncate(fd,(off_t)len);
    if (rtn == -1) {
      fprintf(stderr,"Can't ftruncate(fd,%ld)\n",len);
      fflush(stderr);
    }
  }
  mem = mmap(NULL,len,
	     PROT_READ|PROT_WRITE,MAP_SHARED|MAP_NORESERVE,fd,0);
  //PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
  close(fd);
  if (mem == NULL) {
    fprintf(stderr,"Failed to mmap shared memory file\n");
    fflush(stderr);
    return NULL;
  }
  return mem;
}

inline void fill_mem(void *buf, size_t buf_size, char *fill,
			    size_t fill_len) {
  size_t i;
  if (fill_len == 1) {
    memset(buf,fill[0],buf_size);
  } else {
    char *cmem = (char *)buf;
    for (i =0; i < buf_size; i += fill_len)
      memcpy(cmem+i,fill,fill_len);
    for (i=0; i < buf_size % fill_len; i++)
      cmem[buf_size - buf_size % fill_len + i] = fill[i];
  }
}
