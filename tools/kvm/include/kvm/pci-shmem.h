#ifndef KVM__PCI_SHMEM_H
#define KVM__PCI_SHMEM_H

#include <linux/types.h>
#include <linux/list.h>

struct kvm;
struct shmem_info;

int pci_shmem__init(struct kvm *self);
int pci_shmem__register_mem(struct shmem_info *si);

#endif
