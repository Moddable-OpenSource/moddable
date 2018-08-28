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

debug:
	make -f $(XS_DIR)/makefiles/lin/xsc.mk
	make -f $(XS_DIR)/makefiles/lin/xsid.mk
	make -f $(XS_DIR)/makefiles/lin/xsl.mk
	make -f serial2xsbug.mk
	make -f simulator.mk
	make -f tools.mk
	$(MODDABLE)/build/bin/lin/debug/mcconfig -d -m -p x-lin $(MODDABLE)/tools/xsbug/manifest.json
	
release:
	make GOAL=release -f $(XS_DIR)/makefiles/lin/xsc.mk
	make GOAL=release -f $(XS_DIR)/makefiles/lin/xsid.mk
	make GOAL=release -f $(XS_DIR)/makefiles/lin/xsl.mk
	make GOAL=release -f serial2xsbug.mk
	make GOAL=release -f simulator.mk
	make GOAL=release -f tools.mk
	$(MODDABLE)/build/bin/lin/release/mcconfig -m -p x-lin $(MODDABLE)/tools/xsbug/manifest.json
	
clean:
	make clean -f $(XS_DIR)/makefiles/lin/xsc.mk
	make clean -f $(XS_DIR)/makefiles/lin/xsid.mk
	make clean -f $(XS_DIR)/makefiles/lin/xsl.mk
	make clean -f serial2xsbug.mk
	make clean -f simulator.mk
	make clean -f tools.mk
	
test:
	make -f $(XS_DIR)/makefiles/lin/xst.mk

install:
	make install -f simulator.mk
	make install -f $(BUILD_DIR)/tmp/lin/release/xsbug/makefile
	
