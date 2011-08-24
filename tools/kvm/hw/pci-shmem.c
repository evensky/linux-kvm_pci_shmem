#include "shmem-util.h"
#include "kvm/pci-shmem.h"
#include "kvm/virtio-pci-dev.h"
#include "kvm/irq.h"
#include "kvm/kvm.h"
#include "kvm/pci.h"
#include "kvm/util.h"

static struct pci_device_header pci_shmem_pci_device = {
	.vendor_id = PCI_VENDOR_ID_PCI_SHMEM,
	.device_id = PCI_DEVICE_ID_PCI_SHMEM,
	.header_type = PCI_HEADER_TYPE_NORMAL,
	.class = 0xFF0000,	/* misc pci device */
};

static struct shmem_info *shmem_region;

int pci_shmem__register_mem(struct shmem_info *si)
{
	if (shmem_region == NULL) {
		shmem_region = si;
	} else {
		pr_warning("only single shmem currently avail. ignoring.\n");
		free(si);
	}
	return 0;
}

int pci_shmem__init(struct kvm *kvm)
{
	u8 dev, line, pin;
	char *mem;
	int verbose = 0;
	if (irq__register_device(PCI_DEVICE_ID_PCI_SHMEM, &dev, &pin, &line)
	    < 0)
		return 0;
	/* ignore irq stuff, just want bus info for now. */
	/* pci_shmem_pci_device.irq_pin          = pin; */
	/* pci_shmem_pci_device.irq_line         = line; */
	if (shmem_region == 0) {
		if (verbose == 1)
			pr_warning
			    ("pci_shmem_init: memory region not registered\n");
		return 0;
	}
	pci_shmem_pci_device.bar[0] =
	    shmem_region->phys_addr | PCI_BASE_ADDRESS_SPACE_MEMORY;
	pci_shmem_pci_device.bar_size[0] = shmem_region->size;
	pci__register(&pci_shmem_pci_device, dev);

	mem =
	    setup_shmem(shmem_region->handle, shmem_region->size,
			shmem_region->create, verbose);
	if (mem == NULL)
		return 0;
	kvm__register_mem(kvm, shmem_region->phys_addr, shmem_region->size,
			  mem);
	return 1;
}
