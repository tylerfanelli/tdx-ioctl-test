#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/ioctl.h>

#include <linux/kvm.h>

#include "tdx.h"

static int tdx_caps(int);
static void tdx_caps_print(struct kvm_tdx_capabilities *);

int
main(int argc, char *argv[])
{
	int ret, kvm, vm_fd;

	// Open the KVM device with read + write permissions.
	kvm = open("/dev/kvm", O_RDWR | O_CLOEXEC);
	if (kvm < 0) {
		perror("Error opening KVM device");
		return -1;
	}

	// Create a VM. KVM VM's are represented as files.
	vm_fd = ioctl(kvm, KVM_CREATE_VM, KVM_X86_TDX_VM);
	if (vm_fd < 0) {
		perror("Error creating VM");
		return -1;
	}

	// Print some info about the Intel TDX module.
	ret = tdx_caps(vm_fd);
	if (ret < 0)
		return -1;

	// Clean up VM and KVM device file descriptors.
	close(vm_fd);
	close(kvm);
}

/*
 * Get the capabilities of the Intel TDX module.
 */
static int
tdx_caps(int vm_fd)
{
	int ret, n_cpuid_configs;
	size_t size;
	struct kvm_tdx_cmd cmd;
	struct kvm_tdx_capabilities *caps;

	/*
	 * struct kvm_tdx_capabilities contains a member "cpuid_configs" that
	 * is an array of CPUID config information. In the first generation of
	 * TDX, this ioctl reports 12 different CPUID configs (described by
	 * struct kvm_tdx_cpuid_config). We must therefore allocate space in the
	 * kvm_tdx_capabilities for each of the 12 CPUID configs. The kernel
	 * will then be able to successfully write to this space.
	 */
	n_cpuid_configs = 12;
	
	// We need enough memory for the kvm_tdx_capabilities struct as well as
	// for the 12 CPUID configs.
	size = sizeof(struct kvm_tdx_capabilities);
	size += (sizeof(struct kvm_tdx_cpuid_config) * n_cpuid_configs);

	// Allocate the memory to the kvm_tdx_capabilities struct.
	caps = (struct kvm_tdx_capabilities *) malloc(size);
	if (!caps) {
		perror("Unable to allocate memory for kvm_tdx_capabilities");
		return -1;
	}

	// Set the number of CPUID configs to 12 (required).
	caps->nr_cpuid_configs = n_cpuid_configs;

	/*
	 * Build our KVM TDX command for KVM_TDX_CAPABILITIES.
	 */
	cmd.id = KVM_TDX_CAPABILITIES;	// Issuing a KVM_TDX_CAPABILITIES ioctl.
	cmd.flags = 0;			// flags must be zero.
	cmd.data = (__u64) caps;	// u64 of the pointer to capabilities.
	cmd.error = 0;			// error must be zero.

	/*
	 * Issue the KVM VM ioctl.
	 */
	ret = ioctl(vm_fd, KVM_MEMORY_ENCRYPT_OP, (void *) &cmd);
	if (ret < 0) {
		perror("Unable to fetch TDX capabilities");
		printf("(error = %d)\n", cmd.error);
		return -1;
	}

	tdx_caps_print(caps);

	return 0;
}

/*
 * Print the capabilities of the Intel TDX module (represented by
 * struct kvm_tdx_capabilities).
 */
static void
tdx_caps_print(struct kvm_tdx_capabilities *caps)
{
	int i;
	struct kvm_tdx_cpuid_config cpuid_config;

	printf("\nTDX CAPABILITIES\n================\n");

	printf("attrs_fixed0: %ull\n", caps->attrs_fixed0);
	printf("attrs_fixed1: %ull\n", caps->attrs_fixed1);
	printf("xfam_fixed0: %ull\n", caps->xfam_fixed0);
	printf("xfam_fixed1: %ull\n", caps->xfam_fixed1);
	printf("supported_gpaw: %u\n", caps->supported_gpaw);
	/*
	 * Skip padding and reserved space.
	 */
	printf("nr_cpuid_configs: %u\n", caps->nr_cpuid_configs);

	/*
	 * Print the contents of each CPUID config.
	 */
	for (i = 0; i < caps->nr_cpuid_configs; i++) {
		printf("\nCPUID CONFIG %d\n===============\n", i + 1);
		cpuid_config = caps->cpuid_configs[i];

		printf("leaf: %u\n", cpuid_config.leaf);
		printf("sub_leaf: %u\n", cpuid_config.sub_leaf);
		printf("eax: %u\n", cpuid_config.eax);
		printf("ebx: %u\n", cpuid_config.ebx);
		printf("ecx: %u\n", cpuid_config.ecx);
		printf("edx: %u\n", cpuid_config.edx);
	}
}
