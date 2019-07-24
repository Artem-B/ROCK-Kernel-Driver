#ifndef AMDKCL_DEVICE_CGROUP_H
#define AMDKCL_DEVICE_CGROUP_H
#include <linux/version.h>
#include <linux/types.h>

#if defined(HAVE_BPF_CGROUP_RUN_PROG_DEVICE_CGROUP)
#include <linux/bpf-cgroup.h>
#endif

/*
 * Fix __kcl_devcgroup_check_permission not define error in customer kernel build.
 * the code segment copy from include/linux/device_cgroup.h,
 * directly include the header result build failure since device_cgroup.h has no protection like DEVICE_CGROUP_H.
 */
 #if defined(CONFIG_CGROUP_DEVICE) && !defined(BUILD_AS_DKMS)
extern int __devcgroup_check_permission(short type, u32 major, u32 minor,
					short access);
#endif

#if !defined(HAVE_BPF_CGROUP_RUN_PROG_DEVICE_CGROUP)
#define DEVCG_DEV_CHAR  2
#define DEVCG_ACC_READ  2
#define DEVCG_ACC_WRITE 4
#endif

extern int (*__kcl_devcgroup_check_permission)(short type, u32 major, u32 minor,
				short access);


static inline int kcl_devcgroup_check_permission(short type, u32 major, u32 minor,
					short access)
{
#if defined(HAVE_BPF_CGROUP_RUN_PROG_DEVICE_CGROUP)
	int rc = BPF_CGROUP_RUN_PROG_DEVICE_CGROUP(type, major, minor, access);
        
	if (rc)
		return -EPERM;
#endif

#if defined(CONFIG_CGROUP_DEVICE)
#if defined(BUILD_AS_DKMS)
	return __kcl_devcgroup_check_permission(type, major, minor, access);
#else
	return __devcgroup_check_permission(type, major, minor, access);
#endif
#else
	return 0;
#endif
}

#endif /*AMDKCL_DEVICE_CGROUP_H*/
