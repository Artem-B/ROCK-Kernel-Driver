
LINUXINCLUDE := $(DKMS_INCLUDE_PREFIX) $(LINUXINCLUDE)

ifndef BUILD_AS_DKMS
ccflags-y += -include $(objtree)/$(obj)/dkms/config/config.h
endif

ccflags-y += \
	-I$(TTM_FULL_PATH) \
	-include backport/backport.h
