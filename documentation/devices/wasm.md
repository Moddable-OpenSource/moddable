# Wasm
Copyright 2019-2022 Moddable Tech, Inc.<BR>
Revised: January 27, 2022

This document provides instructions to build and run Moddable SDK apps for the Wasm platform.

![](./../assets/devices/wasm.gif)

## Table of Contents

- Setup instructions

	| [![Apple logo](./../assets/moddable/mac-logo.png)](#mac) | [![Linux logo](./../assets/moddable/lin-logo.png)](#lin) |
	| :---: | :---: |
	| [Setup instructions](#mac) | [Setup instructions](#lin)
	
- [Limitations](#limitations)

<a id="mac"></a>
## macOS

1. If you do not already have the [Emscripten](https://emscripten.org/) repository, clone the repository into your `~/Projects` directory.

	```text
	cd ~/Projects
	git clone https://github.com/emscripten-core/emsdk.git
	```

	If you already have the Emscripten repository, upgrade to the latest version using the following commands:
	
	```text
	cd ~/Projects/emsdk
	git pull
	```

2. Make sure you have all of the Emscripten prerequisites listed for your host platform in the **Platform-specific notes** section of the [Emscripten Download and install webpage](https://emscripten.org/docs/getting_started/downloads.html#platform-notes-installation-instructions-sdk).

3. Install and activate the latest version of Emscripten.
	 
	```text
	cd ~/Projects/emsdk
	./emsdk install latest
	./emsdk activate latest
	```
	
	> We last tested using version 3.1.1 (commit `0f0ea34526515d0b2caa262ab5915bc1a7e5dd71`).
	
4. If you do not already have the [Binaryen](https://github.com/WebAssembly/binaryen) repository, clone the repository into your `~/Projects` directory.

	```text
	cd ~/Projects
	git clone --recursive https://github.com/WebAssembly/binaryen.git
	```
	
	If you already have the Binaryen repository, upgrade to the latest version using the following commands:
	
	```text
	cd ~/Projects/binaryen
	git pull origin main --recurse-submodules
	```
	
	If you experience any problems updating the Binaryen repository, you can simply delete the binaryen directory and re-clone it instead:
	
	```text
	cd ~/Projects
	rm -rf binaryen
	git clone --recursive https://github.com/WebAssembly/binaryen.git
	```

5. Build the Binaryen tools.

	```text
	cd ~/Projects/binaryen
	cmake . && make
	```
	
	> We last tested using wasm-opt version 105 (commit `060442225165d0423d06ea33ab865e850b54f61b`)	
6. 	Setup the `PATH` and other environment variables by pasting the following commands into your `~/.profile`. The first command sources a shell script that sets environment variables for Emscripten. The second updates your `PATH` to include BinaryEn.

	```text
	source ~/Projects/emsdk/emsdk_env.sh
	export PATH=~/Projects/binaryen/bin:$PATH
	```

	> Note: These instructions assume that your shell sources from `~/.profile` when a new terminal is opened. That may not be the case depending on what shell you use and how you have it configured. Starting with macOS Catalina, the [default shell is `zsh`](https://support.apple.com/en-us/HT208050) which uses `~/.zshrc` instead.

	> Note: You must open a new shell instance or manually run the export statements in your shell before proceeding. Adding the export statements to your `~/.profile` does not update the environment variables in active shell instances.

7. Build the Moddable Wasm tools from the command line:

	```text
	cd ${MODDABLE}/build/makefiles/wasm
	make
	```
	
8. To test, build the `balls` example for the `wasm` target.

	```text
	cd $MODDABLE/examples/piu/balls
	mcconfig -d -m -p wasm
	```
	
	You can run the app that's built in the browser by hosting it on a local HTTP server. Python provides a simple tool for doing this.
	
	If you are using Python 2:
	
	```text
	cd $MODDABLE/build/bin/wasm/debug/balls
	python -m SimpleHTTPServer
	```
	
	If you are using Python 3:
	
	```text
	cd $MODDABLE/build/bin/wasm/debug/balls
	python3 -m http.server
	```
	
	Go to [`localhost:8000`](http://localhost:8000) in a browser. You should see a web page with a simulator running `balls`.
	
<a id="lin"></a>
## Linux

1. If you do not already have the [Emscripten](https://emscripten.org/) repository into your `~/Projects` directory and activate the latest version.

	```text
	cd ~/Projects
	git clone https://github.com/emscripten-core/emsdk.git
	cd emsdk
	```

	If you already have the Emscripten repository, upgrade to the latest version using the following commands:
	
	```text
	cd ~/Projects/emsdk
	git pull
	```

2. Make sure you have all of the Emscripten prerequisites listed for your host platform in the **Platform-specific notes** section of the [Emscripten Download and install webpage](https://emscripten.org/docs/getting_started/downloads.html#platform-notes-installation-instructions-sdk).

3. Install and activate the latest version of Emscripten.

	```text
	cd ~/Projects/emsdk
	./emsdk install latest
	./emsdk activate latest
	```
	
	> We last tested using version 3.1.2 (commit `476a14d60d0d25ff5a1bfee18af73a4b9bfbd385`).
		
4. If you do not already have the [Binaryen](https://github.com/WebAssembly/binaryen) repository, clone the repository into your `~/Projects` directory.

	```text
	cd ~/Projects
	git clone --recursive https://github.com/WebAssembly/binaryen.git
	```
	
	If you already have the Binaryen repository, upgrade to the latest version using the following commands:
	
	```text
	cd ~/Projects/binaryen
	git pull origin main --recurse-submodules
	```
	
	If you experience any problems updating the Binaryen repository, you can simply delete the binaryen directory and re-clone it instead:

	```text
	cd ~/Projects
	rm -rf binaryen
	git clone --recursive https://github.com/WebAssembly/binaryen.git
	```

5. Build the Binaryen tools.
	
	```text
	cd ~/Projects/binaryen
	cmake . && make
	```
	
	> We last tested using wasm-opt version 105 (commit `707be2b55075dccaaf0a70e23352c972fce5aa76`)	
6. 	Setup the `PATH` and other environment variables by pasting the following commands into your `~/.bashrc`. The first command sources a shell script that sets environment variables for Emscripten. The second updates your `PATH` to include BinaryEn.

	```text
	source ~/Projects/emsdk/emsdk_env.sh
	export PATH=~/Projects/binaryen/bin:$PATH
	```

	> Note: You must open a new shell instance or manually run the export statements in your shell before proceeding. Adding the export statements to your `~/.profile` does not update the environment variables in active shell instances.

4. Build the Moddable Wasm tools from the command line:

	```text
	cd ${MODDABLE}/build/makefiles/wasm
	make
	```
	
5. To test, build the `balls` example for the `wasm` target.

	```text
	cd $MODDABLE/examples/piu/balls
	mcconfig -d -m -p wasm
	```
	
	You can run the app that's built in the browser by hosting it on a local HTTP server. Python provides a simple tool for doing this.
	
	If you are using Python 2:
	
	```text
	cd $MODDABLE/build/bin/wasm/debug/balls
	python -m SimpleHTTPServer
	```
	
	If you are using Python 3:
	
	```text
	cd $MODDABLE/build/bin/wasm/debug/balls
	python3 -m http.server
	```
	
	Go to [`localhost:8000`](http://localhost:8000) in a browser. You should see a web page with a simulator running `balls`.

<a id="limitations"></a>
## Limitations

Not all features of the Moddable SDK are supported in the Wasm simulator. 

The following features are currently supported:

- All JavaScript features supported by XS
- Piu user interface framework
- Commodetto graphics library

The following features are not supported:

- Files
- Socket
- BLE
- Pins/IO
	
