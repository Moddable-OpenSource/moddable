	@echo "# swiftc" $(<F)
	swiftc $(SWIFT_FLAGS) $< -o $@
