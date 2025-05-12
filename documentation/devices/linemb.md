# Using the Moddable SDK with embedded Linux (Raspberry Pi Zero)
This is a demo document to demostrate how to bring up an embedded Linux device with the Moddable SDK.

# Prepare
check the general get started guide [Moddable SDK - Getting Started](../Moddable SDK - Getting Started.md)
Notice this is only tested with Ubuntu 22.04 for the host machine.

Key steps:
```bash
sudo 
export MODDABLE=[your moddalbe root folder]
export PATH=$PATH:$MODDABLE/build/bin/lin/release
cd $MODDABLE/build/makefiles/lin
make
```

# Install toolchain
There are several CPU types supported. Use the following commands to install the corresponding toolchain:

## ARM(32-bit) - armhf
```bash
sudo apt-get update
sudo apt-get install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf
```

## ARM(64-bit) - arm64
```bash
sudo apt-get update
sudo apt-get install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
```

## amd64(x86_64, the host) - x86_64
```bash
sudo apt-get install build-essential
```

# Sample project
Below are the build commands for different CPU types:

## x86_64 example
```bash
cd $MODDABLE/examples/helloworld
mcconfig -d -m -p linemb/x86_64
```

The generated executable can be found at:
`$MODDABLE/build/bin/linemb/x86_64/debug/helloworld/helloworld`

## Running on ARM devices
### ARM(32-bit)
```bash
cd $MODDABLE/examples/helloworld
mcconfig -d -m -p linemb/armhf
```

The generated executable can be found at:
`$MODDABLE/build/bin/linemb/armhf/debug/helloworld/helloworld`

### ARM(64-bit)
```bash
cd $MODDABLE/examples/helloworld
mcconfig -d -m -p linemb/arm64
```

The generated executable can be found at:
`$MODDABLE/build/bin/linemb/arm64/debug/helloworld/helloworld`

### Copy to device
Find a way to copy this file to the target board (e.g., using scp):

```bash
scp $MODDABLE/build/bin/linemb/armhf/debug/helloworld/helloworld root@172.32.0.93:/root/
```

You should see output like this:
```
# ./helloworld
instruments key: Chunk used,Chunk available,Slot used,Slot available,Stack used,Stack available,Garbage collections,Keys used,Modules loaded,Parser used,Floating Point,Promises settled
Hello, world - sample
instruments: 248,32768,2432,65504,1344,12288,0,2,1,0,0,0
instruments: 248,32768,2432,65504,416,12288,0,2,1,0,0,0
instruments: 248,32768,2432,65504,416,12288,0,2,1,0,0,0
```

# Fix "module unsupported" issue

If you encounter an error like "XXX module unsupported" when using the linemb platform, it's because the module's `manifest.json` file doesn't specify a C language implementation for the linemb platform.

The simplest solution is to copy the implementation from the "lin" platform. Follow these steps:

1. Open the module's `manifest.json` file (e.g., `modules/files/file/manifest.json` for file module)
2. Ensure the linemb platform configuration includes the module implementation, for example:

```json
"linemb": {
    "modules": {
        "*": "$(MODULES)/files/file/lin/*"
    },
    "config": {
        "file": {
            "root": "/tmp/"
        }
    }
}
```

Please note that many modules for the `lin` platform (especially hardware-related ones) have not been thoroughly tested. It's recommended to enable modules according to your specific needs and perform adequate testing to ensure proper functionality.

If you encounter issues with the `lin` platform implementation, a better approach is to create a linemb-specific implementation:

1. Create a new folder for the linemb platform implementation (e.g., `modules/files/file/linemb/`)
2. Implement the necessary C files specifically for the linemb platform
3. Update the manifest.json to use this implementation:

```json
"linemb": {
    "modules": {
        "*": "$(MODULES)/files/file/linemb/*"
    },
    "config": {
        "file": {
            "root": "/tmp/"
        }
    }
}
```

This approach allows you to create optimized implementations tailored to the linemb platform.

