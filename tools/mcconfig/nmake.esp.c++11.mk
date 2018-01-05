	@echo # cc $(@F) (c++11)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(CPP_FLAGS) -std=c++0x $< -o $@
	$(AR) $(AR_OPTIONS) $(LIB_ARCHIVE) $@
