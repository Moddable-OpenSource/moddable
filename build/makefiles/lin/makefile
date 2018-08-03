#
# Copyright (c) 2016-2017  Moddable Tech, Inc.
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
.NOTPARALLEL:

XS_DIR ?= $(MODDABLE)/xs
export XS_DIR
BUILD_DIR ?= $(MODDABLE)/build
export BUILD_DIR

all: debug release

xsc-debug:
	$(MAKE) -f $(XS_DIR)/makefiles/lin/xsc.mk
xsid-debug:
	$(MAKE) -f $(XS_DIR)/makefiles/lin/xsid.mk
xsl-debug:
	$(MAKE) -f $(XS_DIR)/makefiles/lin/xsl.mk
serial2xsbug-debug:
	$(MAKE) -f serial2xsbug.mk
simulator-debug:
	$(MAKE) -f simulator.mk
tools-debug:
	$(MAKE) -f tools.mk
debug: xsc-debug xsid-debug xsl-debug serial2xsbug-debug simulator-debug tools-debug
	$(MODDABLE)/build/bin/lin/debug/mcconfig -d -m -p x-lin $(MODDABLE)/tools/xsbug/manifest.json

xsc-release:
	$(MAKE) GOAL=release -f $(XS_DIR)/makefiles/lin/xsc.mk
xsid-release:
	$(MAKE) GOAL=release -f $(XS_DIR)/makefiles/lin/xsid.mk
xsl-release:
	$(MAKE) GOAL=release -f $(XS_DIR)/makefiles/lin/xsl.mk
serial2xsbug-release:
	$(MAKE) GOAL=release -f serial2xsbug.mk
simulator-release:
	$(MAKE) GOAL=release -f simulator.mk
tools-release:
	$(MAKE) GOAL=release -f tools.mk
release: xsc-release xsid-release xsl-release serial2xsbug-release simulator-release tools-release
	$(MODDABLE)/build/bin/lin/release/mcconfig -m -p x-lin $(MODDABLE)/tools/xsbug/manifest.json

clean:
	$(MAKE) clean -f $(XS_DIR)/makefiles/lin/xsc.mk
	$(MAKE) clean -f $(XS_DIR)/makefiles/lin/xsid.mk
	$(MAKE) clean -f $(XS_DIR)/makefiles/lin/xsl.mk
	$(MAKE) clean -f serial2xsbug.mk
	$(MAKE) clean -f simulator.mk
	$(MAKE) clean -f tools.mk
	rm -rf $(MODDABLE)/build/bin $(MODDABLE)/build/tmp
	
install:
	$(MAKE) install -f simulator.mk
	$(MAKE) install -f $(BUILD_DIR)/tmp/lin/release/xsbug/makefile
