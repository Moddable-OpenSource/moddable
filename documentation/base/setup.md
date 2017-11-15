## setup
Copyright 2017 Moddable Tech, Inc.

Revised: March 8, 2017

### Background
The use of JavaScript on a minimal MCU requires an extremely lightweight runtime. One obvious way to keep the runtime small is to only include functionality which is required by the application being run. For example, while a modern Linux, macOS, or Windows distribution includes drivers for a wide variety of displays to support whatever the user may connect, an efficient MCU distribution only includes a single display driver to support the built-in display of the device. Consequently, each device configuration (MCU + display + network capabilities, etc) is distinct.

While an infinite number of hardware configurations with supporting software frameworks is possible, in practice a handful of configurations are commonly used. For example, Moddable Zero combines an ESP8266 Wi-Fi module with the ILI9341 display controller, a QVGA display, and an XPT2046 touch screen controller. The manifest defines the modules to include in the firmware. The manifest does not contain instructions for how to instantiate these modules, their configuration, or how they may appear in JavaScript global variables.

In some deployments, the configuration and instantiation is performed by the application. Often however, it is convenient to share these tasks for a given hardware configuration across applications. This is the case with example code, which should be kept as simple and focused as possible, and early versions of device applications which can accept the default configuration.

### setup module
The configuration and instantiation may be implemented in a setup module. The setup module runs before the main module, preparing the environment the main module expects. For example, there are two set-up modules for Moddable Zero. Both can establish a Wi-Fi connection, retrieve and IP address, and set the real time clock before the main module runs. Only one contains support for the Piu framework, instantiating the display and touch modules as required by Piu.

The manifest selects which setup module to include. The setup module is optional. If one is not present, the main module is run as before. If a setup module is present, it is responsible for running the main module after the setup tasks are complete. The following is excerpted from a manifest showing how a particular setup module is selected and mapped to the name "setup":

		"esp": {
			"modules": {
				"setup": "$(BUILD)/devices/esp/setup/setup-piu",
				...
			}
		}

### Preload and unloading

Preloading modules is a technique which reduces the runtime RAM used by a module and speeds module load time. To allow the setup and main modules to be preloaded, they may optionally export a function as their module default export. The function is called immediately after the module is loaded. The following is a simple setup module that exports a function that uses this technique:

	export default function () {
		let main = require.weak("main");
		if ("function" === typeof main)
			main.call(this);
	}

By convention, the setup and main modules are loaded with `require.weak` to allow them to be unloaded by the garbage collector when they are no longer in use.

### Additional notes

- The idea for the setup module is loosely modeled on the setup function in the Arduino application model.
- The ILI9341 driver originally required the pixel format be specified when instantiated. This requirement led to a proliferation of screen modules, one for each pixel format. The ILI9341 module has been changed to make the pixel format optional. If it is not provided, the default Commodetto bitmap format is used.
- It may be useful to think of the setup module as providing a sub-platform to the the device target (e.g. ESP + WiFi, ESP + Piu, ESP + Piu + WiFi). It seems possibly interesting to expose this through the mcconfig command line (e.g. -p esp.wifi). To start, the manifest will suffice.
- setup is only implemented for ESP8266. Next up is ESP32. If the approach continues to seem reasonable, we can apply it to other devices.
- The simulator implicitly performs the work of setup already, initializing the screen global variable before running the main module.
