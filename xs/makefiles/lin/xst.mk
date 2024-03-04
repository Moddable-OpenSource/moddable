#
# Copyright (c) 2016-2022  Moddable Tech, Inc.
#
#   This file is part of the Moddable SDK Tools.
# 
#   The Moddable SDK Tools is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
# 
#   The Moddable SDK Tools is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
# 
#   You should have received a copy of the GNU General Public License
#   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
#

% : %.c
%.o : %.c

GOAL ?= debug
NAME = xst
ifneq ($(VERBOSE),1)
MAKEFLAGS += --silent
endif

XS_DIR ?= $(realpath ../..)
BUILD_DIR ?= $(realpath ../../../build)

FUZZILLI ?= 0
OSSFUZZ ?= 0
OSSFUZZ_JSONPARSE ?= 0
FUZZING ?= 0

BIN_DIR = $(BUILD_DIR)/bin/lin/$(GOAL)
INC_DIR = $(XS_DIR)/includes
PLT_DIR = $(XS_DIR)/platforms
SRC_DIR = $(XS_DIR)/sources
TLS_DIR = $(XS_DIR)/tools
TMP_DIR = $(BUILD_DIR)/tmp/lin/$(GOAL)/$(NAME)

ARCH := $(shell uname -m)
LINK_OPTIONS = -rdynamic

C_OPTIONS = \
	-fno-common \
	-DINCLUDE_XSPLATFORM \
	-DXSPLATFORM=\"xst.h\" \
	-DmxAliasInstance=0 \
	-DmxCanonicalNaN=1 \
	-DmxDebug=1 \
	-DmxExplicitResourceManagement=1 \
	-DmxKeysGarbageCollection=1 \
	-DmxLockdown=1 \
	-DmxNoConsole=1 \
	-DmxParse=1 \
	-DmxProfile=1 \
	-DmxRun=1 \
	-DmxSloppy=1 \
	-DmxSnapshot=1 \
	-DmxRegExpUnicodePropertyEscapes=1 \
	-DmxStringNormalize=1 \
	-DmxMinusZero=1 \
	-D_IEEE_LIBM \
	-D__LITTLE_ENDIAN \
	-I$(INC_DIR) \
	-I$(PLT_DIR) \
	-I$(SRC_DIR) \
	-I$(TLS_DIR) \
	-I$(TLS_DIR)/fdlibm \
	-I$(TLS_DIR)/yaml \
	-I$(TMP_DIR)
C_OPTIONS += \
	-Wno-misleading-indentation \
	-Wno-implicit-fallthrough
ifeq ($(GOAL),debug)
	C_OPTIONS += -DmxASANStackMargin=131072
	ifneq ($(FUZZING),0)
		C_OPTIONS += -DmxStress=1
		C_OPTIONS += -DFUZZING=1
	endif
	ifneq ($(OSSFUZZ),0)
		C_OPTIONS += -DOSSFUZZ=1 -DmxMetering=1
		ifneq ($(FUZZ_METER),0)
			C_OPTIONS += -DmxFuzzMeter=$(FUZZ_METER)
		endif
		C_OPTIONS += $(CFLAGS)
		LINK_OPTIONS += $(CXXFLAGS)
		ifneq ($(OSSFUZZ_JSONPARSE),0)
			C_OPTIONS += -DOSSFUZZ_JSONPARSE=1
		endif
	else
		C_OPTIONS += -g -O0 -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter 
		# use asan by default, unless another sanitizer is
		# requested via sanitizer
		ifeq ($(SANITIZER), memory)
			C_OPTIONS += -fsanitize=memory -fsanitize-memory-track-origins
			LINK_OPTIONS += -fsanitize=memory
		else ifeq ($(SANITIZER), undefined)
			C_OPTIONS += -fsanitize=bool,builtin,enum,float-divide-by-zero,integer-divide-by-zero,null,object-size,return,returns-nonnull-attribute,shift,signed-integer-overflow,unreachable,vla-bound,vptr -fno-sanitize-recover=bool,builtin,enum,float-divide-by-zero,integer-divide-by-zero,null,object-size,return,returns-nonnull-attribute,shift,signed-integer-overflow,unreachable,vla-bound,vptr
			LINK_OPTIONS += -fsanitize=bool,builtin,enum,float-divide-by-zero,integer-divide-by-zero,null,object-size,return,returns-nonnull-attribute,shift,signed-integer-overflow,unreachable,vla-bound,vptr -fno-sanitize-recover=bool,builtin,enum,float-divide-by-zero,integer-divide-by-zero,null,object-size,return,returns-nonnull-attribute,shift,signed-integer-overflow,unreachable,vla-bound,vptr

			# function and other ubsan checks not available in some arch, such as arm
			ifneq ($(ARCH), aarch64)
				C_OPTIONS += -fsanitize=array-bounds,function,unsigned-integer-overflow
				LINK_OPTIONS += -fsanitize=array-bounds,function,unsigned-integer-overflow
			endif
		else
			C_OPTIONS += -fsanitize=address
			LINK_OPTIONS += -fsanitize=address
		endif
		C_OPTIONS += -fno-omit-frame-pointer
		LINK_OPTIONS += -fno-omit-frame-pointer
	endif

	ifneq ($(FUZZILLI),0)
		C_OPTIONS += -DFUZZILLI=1 -fsanitize-coverage=trace-pc-guard
	endif
else
	C_OPTIONS += -DmxMultipleThreads=1 -O3
endif

LIBRARIES = -ldl -lm -lpthread -latomic
ifneq ($(FUZZING),0)
	LIBRARIES += -lrt
endif

OBJECTS = \
	$(TMP_DIR)/xsAll.o \
	$(TMP_DIR)/xsAPI.o \
	$(TMP_DIR)/xsArguments.o \
	$(TMP_DIR)/xsArray.o \
	$(TMP_DIR)/xsAtomics.o \
	$(TMP_DIR)/xsBigInt.o \
	$(TMP_DIR)/xsBoolean.o \
	$(TMP_DIR)/xsCode.o \
	$(TMP_DIR)/xsCommon.o \
	$(TMP_DIR)/xsDataView.o \
	$(TMP_DIR)/xsDate.o \
	$(TMP_DIR)/xsDebug.o \
	$(TMP_DIR)/xsDefaults.o \
	$(TMP_DIR)/xsError.o \
	$(TMP_DIR)/xsFunction.o \
	$(TMP_DIR)/xsGenerator.o \
	$(TMP_DIR)/xsGlobal.o \
	$(TMP_DIR)/xsJSON.o \
	$(TMP_DIR)/xsLexical.o \
	$(TMP_DIR)/xsLockdown.o \
	$(TMP_DIR)/xsMapSet.o \
	$(TMP_DIR)/xsMarshall.o \
	$(TMP_DIR)/xsMath.o \
	$(TMP_DIR)/xsMemory.o \
	$(TMP_DIR)/xsModule.o \
	$(TMP_DIR)/xsNumber.o \
	$(TMP_DIR)/xsObject.o \
	$(TMP_DIR)/xsPlatforms.o \
	$(TMP_DIR)/xsProfile.o \
	$(TMP_DIR)/xsPromise.o \
	$(TMP_DIR)/xsProperty.o \
	$(TMP_DIR)/xsProxy.o \
	$(TMP_DIR)/xsRegExp.o \
	$(TMP_DIR)/xsRun.o \
	$(TMP_DIR)/xsScope.o \
	$(TMP_DIR)/xsScript.o \
	$(TMP_DIR)/xsSourceMap.o \
	$(TMP_DIR)/xsString.o \
	$(TMP_DIR)/xsSymbol.o \
	$(TMP_DIR)/xsSyntaxical.o \
	$(TMP_DIR)/xsTree.o \
	$(TMP_DIR)/xsType.o \
	$(TMP_DIR)/xsdtoa.o \
	$(TMP_DIR)/xsre.o \
	$(TMP_DIR)/api.o \
	$(TMP_DIR)/dumper.o \
	$(TMP_DIR)/emitter.o \
	$(TMP_DIR)/loader.o \
	$(TMP_DIR)/parser.o \
	$(TMP_DIR)/reader.o \
	$(TMP_DIR)/scanner.o \
	$(TMP_DIR)/writer.o \
	$(TMP_DIR)/xsmc.o \
	$(TMP_DIR)/textdecoder.o \
	$(TMP_DIR)/textencoder.o \
	$(TMP_DIR)/modBase64.o \
	$(TMP_DIR)/xst.o \
	$(TMP_DIR)/e_acos.o \
	$(TMP_DIR)/e_acosh.o \
	$(TMP_DIR)/e_asin.o \
	$(TMP_DIR)/e_atan2.o \
	$(TMP_DIR)/e_atanh.o \
	$(TMP_DIR)/e_cosh.o \
	$(TMP_DIR)/e_exp.o \
	$(TMP_DIR)/e_fmod.o \
	$(TMP_DIR)/e_hypot.o \
	$(TMP_DIR)/e_log.o \
	$(TMP_DIR)/e_log10.o \
	$(TMP_DIR)/e_pow.o \
	$(TMP_DIR)/e_rem_pio2.o \
	$(TMP_DIR)/e_sinh.o \
	$(TMP_DIR)/k_cos.o \
	$(TMP_DIR)/k_rem_pio2.o \
	$(TMP_DIR)/k_sin.o \
	$(TMP_DIR)/k_tan.o \
	$(TMP_DIR)/s_asinh.o \
	$(TMP_DIR)/s_atan.o \
	$(TMP_DIR)/s_cos.o \
	$(TMP_DIR)/s_expm1.o \
	$(TMP_DIR)/s_ilogb.o \
	$(TMP_DIR)/s_log1p.o \
	$(TMP_DIR)/s_logb.o \
	$(TMP_DIR)/s_scalbn.o \
	$(TMP_DIR)/s_sin.o \
	$(TMP_DIR)/s_tan.o \
	$(TMP_DIR)/s_tanh.o

VPATH += $(SRC_DIR) $(TLS_DIR) $(TLS_DIR)/fdlibm $(TLS_DIR)/yaml
VPATH += $(MODDABLE)/modules/data/text/decoder
VPATH += $(MODDABLE)/modules/data/text/encoder
VPATH += $(MODDABLE)/modules/data/base64

build: $(TMP_DIR) $(BIN_DIR) $(BIN_DIR)/$(NAME)

$(TMP_DIR):
	mkdir -p $(TMP_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/$(NAME): $(OBJECTS)
	@echo "#" $(NAME) $(GOAL) ": cc" $(@F)
ifneq ($(OSSFUZZ),0)
	@echo $(CXX) $(LIB_FUZZING_ENGINE) $(LINK_OPTIONS) $(OBJECTS) $(LIBRARIES) -o $@
	$(CXX) $(LIB_FUZZING_ENGINE) $(LINK_OPTIONS) $(OBJECTS) $(LIBRARIES) -o $@
else
		$(CC) $(LINK_OPTIONS) $(OBJECTS) $(LIBRARIES) -o $@
endif

$(OBJECTS): $(TLS_DIR)/xst.h
$(OBJECTS): $(PLT_DIR)/xsPlatform.h
$(OBJECTS): $(SRC_DIR)/xsCommon.h
$(OBJECTS): $(SRC_DIR)/xsAll.h
$(OBJECTS): $(SRC_DIR)/xsScript.h
$(TMP_DIR)/%.o: %.c
	@echo "#" $(NAME) $(GOAL) ": cc" $(<F)
	@echo $(CC) $< $(C_OPTIONS) -c -o $@
	$(CC) $< $(C_OPTIONS) -c -o $@

clean:
	rm -rf $(BUILD_DIR)/bin/lin/debug/$(NAME)
	rm -rf $(BUILD_DIR)/bin/lin/release/$(NAME)
	rm -rf $(BUILD_DIR)/tmp/lin/debug/$(NAME)
	rm -rf $(BUILD_DIR)/tmp/lin/release/$(NAME)
