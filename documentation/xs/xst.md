# xst

`xst` is the XS test engine, a JavaScript engine to test XS on Linux, macOS and Windows.

## Build

### Linux 

	cd $MODDABLE/xs/makefiles/lin
	make

### macOS 

	cd $MODDABLE/xs/makefiles/mac
	make
	
### Windows 

	cd %MODDABLE%\xs\makefiles\win
	build

## Download

You can use the [jsvu CLI](https://github.com/GoogleChromeLabs/jsvu) to install or update  **xst**.  

You can also download the latest versions of `xst` 
from the [moddable-xst](https://github.com/Moddable-OpenSource/moddable-xst/releases) repository into a directory that is on your `PATH`


## Usage

	xst [-h] [-e] [-m] [-s] [-v] strings...

- `-h`: print this help message
- `-e`: eval `strings`
- `-m`: `strings` are paths to modules
- `-s`: `strings` are paths to scripts
- `-v`: print XS version

Without the `-e`, `-m` or `-s` options, `strings` are paths to **test262** cases or directories. 

### eshost

To test XS with **eshost**, install the [eshost CLI](https://github.com/bterlson/eshost-cli). Then add XS to the hosts:

	eshost --add 'XS' xs ~/.jsvu/xst

**eshost** uses the `-s` option of **xst**.

### test262

To test XS with **test262**, clone [test262](https://github.com/tc39/test262) and change the directory to the `test` directory inside the `test262` directory. For instance:
	
	cd ~/test262/test
	xst language/block-scope
	xst built-ins/TypedArrays/buffer-arg-*

See [XS Conformance](https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/xs/XS%20Conformance.md) for details about how **XS** currently passes **test262** cases.
