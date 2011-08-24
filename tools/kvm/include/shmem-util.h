#ifndef SHMEM_UTIL_H
#define SHMEM_UTIL_H

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#define SHMEM_DEFAULT_SIZE (16 << MB_SHIFT)
#define SHMEM_DEFAULT_ADDR (0xc8000000)
#define SHMEM_DEFAULT_HANDLE "/kvm_shmem"
struct shmem_info {
	uint64_t phys_addr;
	uint64_t size;
	char *handle;
	int create;
};

inline void *setup_shmem(const char *key, size_t len, int creating,
			 int verbose);
inline void fill_mem(void *buf, size_t buf_size, char *fill, size_t fill_len);

#endif				/* SHMEM_UTIL_H */
