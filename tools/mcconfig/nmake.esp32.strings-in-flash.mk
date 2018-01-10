	@echo # cc $(@F) (strings in flash)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $< -o $@.unmapped
	$(OBJCOPY) --rename-section .rodata.str1.1=.data $@.unmapped $@
