	@echo # cc $(@F) (strings in flash)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $< -o $@.unmapped
	$(TOOLS_BIN)\xtensa-lx106-elf-objcopy --rename-section .rodata.str1.1=.irom0.str.1 --rename-section .text=irom0.code $@.unmapped $@ 
	$(AR) $(AR_OPTIONS) $(APP_ARCHIVE) $@

