# Bosch BME68x Visualizers
Updated: January 24, 2024

This directory contains four different visualizers for the Bosh BME68x sensor. The sensor provides temperature, humidity, and air pressure.

## Requirements
The project is intended to run on the ESP32 family of products. It also on Raspberry Pi Pico. It requires a screen. The visualizer expects a round screen with a diameter of 240 pixels. It will run on a square or rectangular screen, though some drawing artifacts may be visible outside the area of of the circle.

The project expects a BME68x sensor to be connected to the default I2C bus of the device. If it is not, exceptions will be thrown on launch. The project does contain default values for the BME68x, so if you press "Go" in the debugger on these exceptions, it will display the selected visualizer. The simulator includes controls to adjust the simulator values.

The BME68x sensor driver conforms to [ECMA-419](https://419.ecma-international.org). This means that the target device must include ECMA-419 IO support. For discussion purposes, these notes use the `esp32/moddable_two_io` target.

## Building
This project consists of a host that is installed first. It includes the JavaScript engine, Piu user interface framework, outline graphics rendering, ECMA-419 IO, and BME68x sensor driver. 

The visualizers use the `fontbm` command line tool prepare fonts as part of the build. If you use xs-dev to install and update the Moddable SDK, you can install `fontbm` with:

```
xs-dev setup --tool fontbm
```

Otherwise, see "fontbm instructions" in "[Creating fonts for applications built on the Moddable SDK](https://www.moddable.com/documentation/commodetto/Creating%20fonts%20for%20Moddable%20applications)`"

### Simulator

To build an install the host on the simulator:

```
cd $MODDABLE/examples/piu/round-bme68x
mcconfig -d -m
```

**Important**: Leave the simulator running after installing the host. This is required by the next step.

Once installed, the host will display a message that it is ready for a mod. The visualizers are mods. To install and run a visualizer, use one of the following commands:

```
mcrun -d -m ./mods/plain/manifest.json
mcrun -d -m ./mods/bankers/manifest.json
mcrun -d -m ./mods/gauges/manifest.json
mcrun -d -m ./mods/histograms/manifest.json
```

### Device
To build an install the host on a device, use the following command line. This uses the Moddable Two device. If you are using another device, replace `esp32/moddable_two_io` with the platform target for your device.


```
cd $MODDABLE/examples/piu/round-bme68x
mcconfig -d -m -p esp32/moddable_two_io
```

Once installed, the host will display a message that it is ready for a mod. The visualizers are mods. To install and run a visualizer, use one of the following commands:

```
mcrun -d -m -p esp32/moddable_two_io ./mods/plain/manifest.json
mcrun -d -m -p esp32/moddable_two_io ./mods/bankers/manifest.json
mcrun -d -m -p esp32/moddable_two_io ./mods/gauges/manifest.json
mcrun -d -m -p esp32/moddable_two_io ./mods/histograms/manifest.json
```

Press the "flash" button on the ESP32 to change the appearance of the visualizers.

> **Note**: Once a mod is installed, you can replace it by installing a different mod. You do no need to reinstall the host between installing mods.
