#
# Copyright 2017 Advanced Micro Devices, Inc.
# Copyright 2019 Raptor Engineering, LLC
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
# OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.
#
#
# Makefile for the 'calcs' sub-component of DAL.
# It calculates Bandwidth and Watermarks values for HW programming
#

ifeq (1,$(AMDKCL_KBUILD_TAKE_PATH))
AMDKCL_KBUILD_TAKE_PATH_CURRENT=$(AMDDALPATH)/dc/calcs/
else
AMDKCL_KBUILD_TAKE_PATH_CURRENT=
endif

ifdef CONFIG_X86
calcs_ccflags := -mhard-float -msse
endif

ifdef CONFIG_PPC64
calcs_ccflags := -mhard-float -maltivec
endif

ifeq ($(call cc-ifversion, -lt, 0701, y), y)
IS_OLD_GCC = 1
endif

ifdef CONFIG_X86
ifdef IS_OLD_GCC
# Stack alignment mismatch, proceed with caution.
# GCC < 7.1 cannot compile code using `double` and -mpreferred-stack-boundary=3
# (8B stack alignment).
calcs_ccflags += -mpreferred-stack-boundary=4
else
calcs_ccflags += -msse2
endif
endif

CFLAGS_$(AMDKCL_KBUILD_TAKE_PATH_CURRENT)dcn_calcs.o := $(calcs_ccflags)
CFLAGS_$(AMDKCL_KBUILD_TAKE_PATH_CURRENT)dcn_calc_auto.o := $(calcs_ccflags)
CFLAGS_$(AMDKCL_KBUILD_TAKE_PATH_CURRENT)dcn_calc_math.o := $(calcs_ccflags) -Wno-tautological-compare

BW_CALCS = dce_calcs.o bw_fixed.o custom_float.o

ifdef CONFIG_DRM_AMD_DC_DCN1_0
BW_CALCS += dcn_calcs.o dcn_calc_math.o dcn_calc_auto.o
endif

AMD_DAL_BW_CALCS = $(addprefix $(AMDDALPATH)/dc/calcs/,$(BW_CALCS))

AMD_DISPLAY_FILES += $(AMD_DAL_BW_CALCS)
