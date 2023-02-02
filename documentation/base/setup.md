# Setup
Copyright 2017-2023 Moddable Tech, Inc.<BR>
Revised: February 1, 2023

## Background
The use of JavaScript on a minimal MCU requires an extremely lightweight runtime. One obvious way to keep the runtime small is to only include functionality which is required by the application being run. For example, while a modern Linux, macOS, or Windows distribution includes drivers for a wide variety of displays to support whatever the user may connect, an efficient MCU distribution only includes a single display driver to support the built-in display of the device. Consequently, each device configuration (MCU + display + network capabilities, etc) is distinct.

While an infinite number of hardware configurations with supporting software frameworks is possible, in practice a handful of configurations are commonly used. For example, Moddable Zero combines an ESP8266 Wi-Fi module with the ILI9341 display controller, a QVGA display, and an XPT2046 resistive touch screen controller. The manifest defines the modules to include in the firmware. The manifest does not contain instructions for how to instantiate these modules, their configuration, or how they may appear in JavaScript global variables.

In some deployments, the configuration and instantiation is performed by the application. Often however, it is convenient to share these tasks for a given hardware configuration across applications. This is the case with example code, which should be kept as simple and focused as possible, and early versions of device applications which can accept the default configuration.

### setup module
The configuration and instantiation may be implemented in one or more setup modules. Set up modules are identified by having a module specified that begins with `setup/`. All setup modules run before the main module, preparing the environment the main module requires. For example, there are two primary set-up modules for Moddable Zero. The `setup/network` module establishes a Wi-Fi connection, retrieves an IP address, and sets the real time clock. The `setup/piu` module instantiates the display and touch drivers and binds them to the global variable `screen`.

Usually applications do not need to explicitly include the setup modules in their manifest as they are included by the manifests that require them. For example the `example/manifest_network.json` manifest includes `setup/network`, and the `example/manifest_piu.json` manifest includes `setup/piu`.

The implementation of a setup is straightforward. The module exports a single function to perform. This function is called at start-up time. When there are multiple setup modules, there is no guarantee on the order in which they are called.

The setup function receives a single argument, a done function to call when setup is complete. The `done` function allows setup to perform asynchronous operations, such as establishing a network connection.

	export default function (done) {
		setupOperation();
		done();
	}

## Preload

To minimize RAM use and speed start-up time, the main module and any setup modules should be [preloaded](../xs/preload.md).

## Additional notes

- The idea for setup modules is loosely modeled on the setup function in the Arduino application model. 
- The simulator implicitly performs the work of setup already, initializing the screen global variable before running the main module.
