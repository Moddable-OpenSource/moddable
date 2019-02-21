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
#

GOAL ?= debug
NAME = simulator
ifneq ($(VERBOSE),1)
MAKEFLAGS += --silent
endif

BUILD_DIR ?= $(realpath ../..)

BIN_DIR = $(BUILD_DIR)/bin/lin/$(GOAL)
TMP_DIR = $(BUILD_DIR)/tmp/lin/$(GOAL)/$(NAME)

SIMULATOR = $(MODDABLE)/build/simulator

PKGCONFIG = $(shell which pkg-config)
GLIB_COMPILE_RESOURCES = $(shell $(PKGCONFIG) --variable=glib_compile_resources gio-2.0)

C_OPTIONS = $(shell $(PKGCONFIG) --cflags gtk+-3.0)
ifeq ($(GOAL),debug)
	C_OPTIONS += -g -O0 -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter
else
	C_OPTIONS += -O3
endif

LINK_OPTIONS = $(shell $(PKGCONFIG) --libs gtk+-3.0)

LIBRARIES = -ldl

build: $(TMP_DIR) $(BIN_DIR)/$(NAME)

$(TMP_DIR):
	mkdir -p $(TMP_DIR)
	
$(BIN_DIR)/$(NAME): $(TMP_DIR)/main.o $(TMP_DIR)/resources.o
	@echo "#" $(NAME) $(GOAL) ": link simulator"
	$(CC) $(TMP_DIR)/main.o $(TMP_DIR)/resources.o $(LINK_OPTIONS) $(LIBRARIES) -o $@ 
	
$(TMP_DIR)/main.o: $(SIMULATOR)/lin/main.c $(SIMULATOR)/screen.h
	@echo "#" $(NAME) $(GOAL) ": cc" $(<F)
	$(CC) $< $(C_OPTIONS) -c -o $@
	
$(TMP_DIR)/resources.o: $(TMP_DIR)/resources.c
	@echo "#" $(NAME) $(GOAL) ": cc" $(<F)
	$(CC) $< $(C_OPTIONS) -c -o $@

$(TMP_DIR)/resources.c: $(SIMULATOR)/lin/main.gresource.xml
	@echo "#" $(NAME) $(GOAL) ": glib-compile-resources" $(<F)
	cp $(SIMULATOR)/screens/* $(TMP_DIR)
	cp $(SIMULATOR)/lin/main.gresource.xml $(TMP_DIR)/resources.xml
	cd $(TMP_DIR); $(GLIB_COMPILE_RESOURCES) --generate-source --c-name screens resources.xml
	
clean:
	rm -rf $(BUILD_DIR)/bin/lin/debug/$(NAME)
	rm -rf $(BUILD_DIR)/bin/lin/release/$(NAME)
	rm -rf $(BUILD_DIR)/tmp/lin/debug/$(NAME)
	rm -rf $(BUILD_DIR)/tmp/lin/release/$(NAME)


APP_DIR = /usr/bin
DESKTOP_DIR = /usr/share/applications/
ICON_DIR = /usr/share/icons/hicolor

APP = tech-moddable-simulator
ICONS = $(foreach SIZE,32 48 64 96 128 256,$(ICON_DIR)/$(SIZE)x$(SIZE)/apps/$(APP).png)

install: $(APP_DIR)/$(APP) $(DESKTOP_DIR)/$(APP).desktop $(ICONS)
	sudo gtk-update-icon-cache -f -t $(ICON_DIR)

$(APP_DIR)/$(APP): $(BIN_DIR)/$(NAME)
	@echo "#" $(NAME) $(GOAL) ": cp" $(<F)
	sudo cp $< $@

$(DESKTOP_DIR)/$(APP).desktop: $(SIMULATOR)/lin/main.desktop
	@echo "#" $(NAME) $(GOAL) ": cp" $(<F)
	sudo cp $< $@

$(ICON_DIR)/%/apps/$(APP).png: $(SIMULATOR)/lin/icons/%.png
	@echo "#" $(NAME) $(GOAL) ": cp" $(<F)
	sudo cp $< $@
