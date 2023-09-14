#	@echo "# separate assemble: cc $(<F) "
#	$(CC) $(C_FLAGS) $(C_INCLUDES) $(C_DEFINES) $< -o $@.asm
#	$(AS) $(NRF_ASM_FLAGS) $@.asm -o $@
	@echo "# separate assemble: cc" $(<F) "(strings in flash)"
	$(CC) $(C_FLAGS) $(C_INCLUDES) $(C_DEFINES) $< -o $@.asm
	$(AS) $(NRF_ASM_FLAGS) $@.asm -o $@.unmapped
	$(OBJCOPY) --rename-section .rodata.str1.1=.data $@.unmapped $@

