export MODDABLE := $(HOME)/bindmounts/trees/moddable
export PATH := $(MODDABLE)/build/bin/lin/release:$(PATH)

do-build:
	cd examples/cli && mcconfig -p x-cli-lin
	cd build/tmp/lin/release/cli && $(MAKE)
	./build/bin/lin/release/cli

clean:
	-rm -r build/tmp/lin/release/cli
	-rm build/bin/lin/release/cli

