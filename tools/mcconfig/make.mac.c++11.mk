	@echo "# cc" $(<F) "(c++11)"
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -std=c++11 $< -o $@
