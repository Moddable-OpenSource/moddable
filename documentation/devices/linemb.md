# Using the Moddable SDK with embedded Linux (Raspberry Pi Zero)
This is a demo document to demostration how to bring up an embedded Linux device with the Moddable SDK.

# Prepare
check the general get started guide [Moddable SDK - Getting Started](../Moddable SDK - Getting Started.md)
Notice this is only tested with Ubuntu 22.04 for the host machine.

Key steps:
```bash
sudo 
export MODDABLE=/workspaces/linfan/moddable
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

## ARM(32-bit) example
```bash
cd $MODDABLE/examples/helloworld
mcconfig -d -m -p linemb/armhf
```

The generated executable can be found at:
`$MODDABLE/build/bin/linemb/armhf/debug/helloworld/helloworld`

## ARM(64-bit) example
```bash
cd $MODDABLE/examples/helloworld
mcconfig -d -m -p linemb/arm64
```

The generated executable can be found at:
`$MODDABLE/build/bin/linemb/arm64/debug/helloworld/helloworld`

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