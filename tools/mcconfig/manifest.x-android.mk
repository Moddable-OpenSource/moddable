#
# NEEDS BOILERPLATE
#     Copyright (C) 2016-2017 Moddable Tech, Inc.
#     All rights reserved.
#
	
XSC = $(BUILD_DIR)/bin/mac/debug/xsc
XSID = $(BUILD_DIR)/bin/mac/debug/xsid
XSL = $(BUILD_DIR)/bin/mac/debug/xsl

.PHONY: all	

all: $(TMP_DIR)/mc.xs.c $(SOURCES) $(RESOURCES)
	cd $(PROJECT); ./gradlew installX86Debug

$(TMP_DIR)/mc.xs.c: $(MODULES) $(MANIFEST)
	@echo "# xsl modules"
	$(XSL) -t -b $(MODULES_DIR) -o $(TMP_DIR) $(PRELOADS) $(CREATION) $(MODULES)
	
MAKEFLAGS += --jobs
ifneq ($(VERBOSE),1)
MAKEFLAGS += --silent
endif
