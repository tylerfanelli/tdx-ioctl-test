#define KVM_X86_TDX_VM		2

/* Trust Domain eXtension sub-ioctl() commands. */
enum kvm_tdx_cmd_id {
	KVM_TDX_CAPABILITIES = 0,
	KVM_TDX_INIT_VM,
	KVM_TDX_INIT_VCPU,
	KVM_TDX_INIT_MEM_REGION,
	KVM_TDX_FINALIZE_VM,

	KVM_TDX_CMD_NR_MAX,
};

struct kvm_tdx_cmd {
	/* enum kvm_tdx_cmd_id */
	__u32 id;
	/* flags for sub-commend. If sub-command doesn't use this, set zero. */
	__u32 flags;
	/*
	 * data for each sub-command. An immediate or a pointer to the actual
	 * data in process virtual address.  If sub-command doesn't use it,
	 * set zero.
	 */
	__u64 data;
	/*
	 * Auxiliary error code.  The sub-command may return TDX SEAMCALL
	 * status code in addition to -Exxx.
	 * Defined for consistency with struct kvm_sev_cmd.
	 */
	__u64 error;
};

struct kvm_tdx_cpuid_config {
	__u32 leaf;
	__u32 sub_leaf;
	__u32 eax;
	__u32 ebx;
	__u32 ecx;
	__u32 edx;
};

struct kvm_tdx_capabilities {
	__u64 attrs_fixed0;
	__u64 attrs_fixed1;
	__u64 xfam_fixed0;
	__u64 xfam_fixed1;
#define TDX_CAP_GPAW_48	(1 << 0)
#define TDX_CAP_GPAW_52	(1 << 1)
	__u32 supported_gpaw;
	__u32 padding;
	__u64 reserved[251];

	__u32 nr_cpuid_configs;
	struct kvm_tdx_cpuid_config cpuid_configs[];
};

struct kvm_tdx_init_vm {
	__u64 attributes;
	__u64 mrconfigid[6];	/* sha384 digest */
	__u64 mrowner[6];	/* sha384 digest */
	__u64 mrownerconfig[6];	/* sha348 digest */
	/*
	 * For future extensibility to make sizeof(struct kvm_tdx_init_vm) = 8KB.
	 * This should be enough given sizeof(TD_PARAMS) = 1024.
	 * 8KB was chosen given because
	 * sizeof(struct kvm_cpuid_entry2) * KVM_MAX_CPUID_ENTRIES(=256) = 8KB.
	 */
	__u64 reserved[1004];

	/*
	 * Call KVM_TDX_INIT_VM before vcpu creation, thus before
	 * KVM_SET_CPUID2.
	 * This configuration supersedes KVM_SET_CPUID2s for VCPUs because the
	 * TDX module directly virtualizes those CPUIDs without VMM.  The user
	 * space VMM, e.g. qemu, should make KVM_SET_CPUID2 consistent with
	 * those values.  If it doesn't, KVM may have wrong idea of vCPUIDs of
	 * the guest, and KVM may wrongly emulate CPUIDs or MSRs that the TDX
	 * module doesn't virtualize.
	 */
	struct kvm_cpuid2 cpuid;
};

#define KVM_TDX_MEASURE_MEMORY_REGION	(1UL << 0)

struct kvm_tdx_init_mem_region {
	__u64 source_addr;
	__u64 gpa;
	__u64 nr_pages;
};
