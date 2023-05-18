# Command line (cli) controller

Extends Moddable to be able to provide command line program using the `cli-<platform>` platform type, including
`cli-win` for Windows, `cli-mac` for Mac, and `cli-lin` for Linux.  Includes support host/mod loading, timers,
promises and workers.

## Running your program

When you build, such as `mcconfig -m -d -p cli-win`, the resulting program will be a native app (such as `.exe` on
windows, or `.app` on mac).  If you run that program with no command line arguments, the default exported `main`
function will be called.  The code will continue to operate, including support for Timer and other async services.  If
you need to terminate the app under control of the program, use:

```
function abort(status) @ "fxAbort";

fxAbort(status);    // status is the exit code the program will terminate with
```

## Using mods

It you specify a path to a mod file (`.xsa` file, built with `mcrun`) as the first argument to the program, it will be
loaded and symbols resolved before the `main` exported function is called.  This is slightly different than how the
microcontrollers consume mods (from the flash `xs` partition, upon reboot) or how the simulators consume mods (provided
via the `mcrun` program while a host waits for the module to be presented), but the resulting functionality remains 
the same.

## Using the debugger and instrumentation

Full support for the debugger and instrumentation is included, and it should behave the same as using other platform
types.  Instrumentation is updated once per second.

## Linux requires X Windows System

The Moddable implementation for Linux depends on the GTK3+ libraries, and as such even though a console application is
built using `cli`, it still depends on X being installed.  This means that running the Linux console application from
`telnet` or `ssh`, or from a server-based (non-desktop) Linux distro will not work.  You should be able to execute it 
by installing X and also using the `xvfb` module (which implements a headless X frame buffer), but this has not been
tested yet.  If you try this and find a recipe that works, please submit a pull and update this documentation
accordingly.  

A great future project for someone is to consider implementing a generic console machine across all of Moddable using
the `libuv` event handler, which is the event handler from Node.  It is cross-platform and should allow for building
command line programs on a wide range of platforms without the need for X or any window manager, assuming other
cross-platform libraries are also consumed in the process for operations like files, resources, etc.
