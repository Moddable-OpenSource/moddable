# Ethernet
Copyright 2021-2026 Moddable Tech, Inc.<BR>
Revised: April 10, 2026

## Overview

The Moddable SDK supports Ethernet for the ESP32. Ethernet support is an extension to our [network support](./network.md), which includes support for Wi-Fi and protocols like HTTP/HTTPS, MQTT, WebSockets, DNS, and mDNS. Most of the [networking examples](../../examples/network) in the Moddable SDK enable Wi-Fi by default; however, examples that work with Wi-Fi can easily be modified to use Ethernet instead.

This document provides information about how to enable Ethernet in applications, details of the JavaScript API used to establish and monitor an Ethernet connection, and wiring instructions for a compatible Ethernet breakout board.

## Table of Contents

* [Configuration](#Configuration)
* [Wiring](#wiring)
* [Enabling Ethernet in applications](#enabling-ethernet)
* [Class Ethernet](#class-ethernet)

<a id="Configuration"></a>

## Configuration

The Moddable SDK supports any Ethernet module that can be wired into the hardware and integrated with the ESP-IDF. We've worked with various [Microchip ENC28J60](https://www.microchip.com/en-us/product/enc28j60) and [WIZnet W5500](https://docs.wiznet.io/Product/iEthernet/W5500/overview) SPI-to-Ethernet module breakout boards and achieved good results.

The [ESP32 processors and their SDK](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/network/esp_eth.html) offer excellent support for Ethernet. They support both internal Ethernet MAC controllers and external SPI-to-Ethernet modules. The [WT32-ETH01](https://www.seeedstudio.com/Ethernet-module-based-on-ESP32-series-WT32-ETH01-p-4736.html?srsltid=AfmBOoq272pKrSmxaVh-1rOldlpPZ_CrkBy1VQq0xNOkn5bPJ7aETZqy) modules use the ESP32's internal MAC and an external PHY chip for Ethernet support (see [$MODDABLE/build/devices/esp32/targets/wt32_eth01](../../build/devices/esp32/targets/wt32_eth01)). You specify `esp32/wt32_eth01` as the device. The [`sdkconfig.defaults`](../tools/manifest.md#sdkconfig) file specifies whether to enable Ethernet and the type of Ethernet module or interface to use. For the WT32-ETH01 to use the internal MAC and external PHY, it requires the target device's `sdkconfig.defaults` file to contain:

```
CONFIG_ETH_ENABLED=y
CONFIG_ETH_USE_ESP32_EMAC=y
CONFIG_ETH_DMA_BUFFER_SIZE=512
CONFIG_ETH_DMA_RX_BUFFER_NUM=10
CONFIG_ETH_DMA_TX_BUFFER_NUM=10
```

For the WIZnet W5500, it requires the `sdkconfig.defaults` in the target processor to contain:

```
CONFIG_ETH_ENABLED=y
CONFIG_ETH_USE_SPI_ETHERNET=y
CONFIG_ETH_SPI_ETHERNET_W5500=y
```

Using the ENC28J80 module with an ESP32, it is installed using the `dependency` feature in `$MODDABLE/modules/network/ethernet/manifest.json` to dynamically load the module and associated files for the device, including the `sdkconfig` values.

```jsonc
"dependency": [
         {
           "name": "enc28j60",
           "version": "^1.0.1",
           "includes": [
             "include"
           ]
         }
       ],
```

The `$MODDABLE/modules/network/ethernet` directory contains a `manifest.json_enc28j60` and `manifest.json_w5500` file. One of these needs to be copied to `manifest.json` to define the Ethernet -SPI setup parameters for the particular chip.

```json
"defines": {
     "defines": {
        "ethernet": {
        	"enc28j60": 1,
        	"w5500": 0,
			"debug": 0,
			"hostname":"\"Moddable\"",
            "hz": 6000000,
            "int_pin": 33,
            "power_pin": 16,
            "spi": {
            	"command_bits": 3,
				"address_bits": 5,
                "cs_pin": 27,
                "port": "SPI3_HOST",
                "miso_pin": 35,
                "mosi_pin": 26,
                "sck_pin": 0,
                "polling_ms": 0,
				"dma_ch": 3
            }
        }
    }
}
```

The drivers in `$MODDABLE/modules/network/ethernet/esp32/drivers` contain the code that calls the setup functions for the Ethernet MAC and PHY components in the Espressif SDK. The main Ethernet code is in `$MODDABLE/modules/network/ethernet/esp32/ethernet.c` and is primarily driven by the values specified in the `manifest.json` file.

The `CMakeLists.txt` file for the target device needs to specify that it includes the `esp_eth` and `esp_netif` Ethernet libraries from the ESP32 SDK. For the `add_prebuilt_library` line, it needs to look similar to

```
add_prebuilt_library(xsesp32 ${CMAKE_BINARY_DIR}/xs_${ESP32_SUBCLASS}.a
add_prebuilt_library(xsesp32 ${CMAKE_BINARY_DIR}/xs_${ESP32_SUBCLASS}.a
	REQUIRES esp_timer esp_wifi spi_flash bt esp_lcd nvs_flash
	spiffs esp_driver_gpio esp_driver_spi esp_eth esp_netif log ${ESP_COMPONENTS}
)
```

Example, for the ESP32-S3, you will find the file in `$MODDABLE/build/devices/esp32/xsProj-esp32s3/main/CMakeLists.txt` and the `sdkconfig.defaults` in the directory above.

<a id="wiring"></a>

## Wiring

This section contains wiring information for ENC28J60 modules (examples pictured below), which is directly applicable to W5500 modules.

<img src="./assets/enc28j60.jpeg" style="zoom:50%;" /> ![](./assets/ENC28J60-Ethernet-Module.jpeg)

The ESP32 communicates with the ENC28J60 over the SPI interface.

| ENC28J60 | ESP32 |
| :---: | :---: |
| INT | GPIO 33 |
| CS | GPIO 27 |
| MISO | GPIO 35 |
| MOSI | GPIO 26 |
| SCK | GPIO 0 |
| PWR | PWR |
| GND | GND |

The diagram below shows a Moddable Two (an ESP32-based hardware module) wired to a HanRun ENC28J60 module.

![](../assets/network/enc28j60-wiring-moddable-two.png)

You can use other ESP32-based development boards too. Many developers use NodeMCU ESP32 boards, pictured in the diagram below. These boards require you to solder to the GPIO 0 pad since they do not have a pinout for GPIO 0 (because GPIO 0 is used by the BOOT button).

![](../assets/network/enc28j60-wiring-nodemcu.png)

<a id="enabling-ethernet"></a>
## Enabling Ethernet in Applications

Most of the [networking examples](../../examples/network) in the Moddable SDK enable Wi-Fi by default. They do this by including the `manifest_net.json` manifest.

```jsonc
"include": [
	/* other included manifests here */
	"$(MODDABLE)/examples/manifest_net.json"
],
```

If you want to use Ethernet in these examples, you can simply replace `manifest_net.json` with `manifest_net_ethernet.json`. Note that you should not include both `manifest_net.json` and `manifest_net_ethernet.json`; only include one or the other.

```jsonc
"include": [
	/* other included manifests here */,
	"$(MODDABLE)/modules/network/ethernet/manifest_net_ethernet.json"
],
```

The `manifest_net_ethernet.json` manifest includes a `setup/network` module that automatically sets up the connection to Ethernet. The connection is set up before the rest of the application runs. In other words, the device connects to Ethernet, then the application's `main` module is executed. If the device is unable to connect to Ethernet, the `main` module is never executed.

You may choose to remove the `setup/network` module from your own applications and instead set up and monitor the Ethernet connection in the application code using the JavaScript APIs described in the [Class Ethernet](#class-ethernet) section below.

### ESP-IDF ENC28J60 Driver Multicast Packet Issue

The ENC28J60 driver in the ESP-IDF contains a bug that causes multicast packets to be filtered out in hardware. Moddable has [fixed this bug via an ESP-IDF pull request](https://github.com/espressif/esp-idf/commit/3e9cdbdedfd47813a55454ff3b9541fb5c9f9a61) that will be included in a future ESP-IDF release. Until that time, if your project needs multicast you can apply this small fix in the file `$IDF_PATH/examples/ethernet/enc28j60/main/esp_eth_mac_enc28j60.c`, lines 551-552:

```diff
-    // set up default filter mode: (unicast OR broadcast) AND crc valid
-    MAC_CHECK(enc28j60_register_write(emac, ENC28J60_ERXFCON, ERXFCON_UCEN | ERXFCON_CRCEN | ERXFCON_BCEN) == ESP_OK,
+    // set up default filter mode: (unicast OR broadcast OR multicast) AND crc valid
+    MAC_CHECK(enc28j60_register_write(emac, ENC28J60_ERXFCON, ERXFCON_UCEN | ERXFCON_CRCEN | ERXFCON_BCEN | ERXFCON_MCEN) == ESP_OK,
```

<a id="class-ethernet"></a>
## Class Ethernet

- **Source code:** [ethernet](../../modules/network/ethernet)
- **Relevant Examples:** [`ethernet-test`](../../examples/network/ethernet/ethernet-test), [`ethernet-test-graphic`](../../examples/network/ethernet/ethernet-test-graphic), [`ethernet-monitor`](../../examples/network/ethernet/ethernet-monitor)

The `Ethernet` class provides access to use and configure the Ethernet capabilities of an Ethernet module.

```js
import Ethernet from "ethernet";
```

The software for Ethernet is nearly identical to Wi-Fi with the exception of establishing a connection. Since Ethernet implements the same API as Wi-Fi, you can mostly use Ethernet as a drop-in replacement to Wi-Fi in examples from the Moddable SDK and in your own applications, rather than rewriting large portions of them.

### `static start()`

The `start` method begins the underlying process to manage the device's connection to the network.

```js
Ethernet.start();
```

### `static useDHCP()`

The `useDHCP` method disables the active static IP address, if any, any activates the DHCP client to maintain the device's IP address.

```js
Ethernet.useDHCP();
```

### `static useStaticIP(address, mask, gateway)`

The `useDHCP` method disables the DHCPclient, if active, any uses the provided arguments for the device's IP address.

```js
Ethernet.useStaticIP("10.0.0.63", "255.255.255.0", "10.0.0.1");
```

### `constructor(callback)`

The `Ethernet` constructor creates a monitor for the Ethernet status. It takes a callback function to receive status messages. Messages are strings representing the state of the Ethernet connection. The `Ethernet` class has properties that correspond to each string as a convenience for developers.

| Property | Description |
| :---: | :--- |
| `Ethernet.connected` | Physical link established
| `Ethernet.disconnected ` | Physical link lost
| `Ethernet.gotIP` | IP address has been assigned via DHCP and network connection is ready for use
| `Ethernet.lostIP ` | IP address has been lost and network connection is no longer usable

> **Note:** Applications must call the static `start` method of the `Ethernet` class to initiate the Ethernet connection. Without calling `start`, the callback passed into the `Ethernet` constructor will never be called.

```js
Ethernet.start();
let monitor = new Ethernet((msg) => {
	switch (msg) {
		case Ethernet.disconnected:
			// Ethernet disconnected
			break;
		case Ethernet.gotIP:
			// Got IP address
			break;
	}
});
```

### `close()`

The `close` method closes the connection between the `Ethernet` instance and the underlying process managing the device's connection to the network. In other words, it prevents future calls to the callback function, but it does not disconnect from the network.

```js
let monitor = new Ethernet((msg) => {
	trace(`Ethernet msg: ${msg}\n`);
});

monitor.close();
```

### Example: Get Ethernet IP address

The following example begins the process of connecting to Ethernet and traces to the console when the connection succeeds with an IP address being assigned to the Ethernet device.

The `Net.get` method can be used to get the IP and MAC addresses of the Ethernet interface, as with Wi-Fi. An optional second argument to `Net.get` specifies which interface to query: `"ethernet"`, `"ap"`, or `"station"`. If no second argument is provided, `Net.get` defaults to the active network interface (for example, when the only network connection is Ethernet, the Ethernet interface is the default).

```js
Ethernet.start();
let monitor = new Ethernet((msg) => {
	switch (msg) {
		case Ethernet.connected:
			trace(`Physical link established. Waiting for IP address.\n`);
			break;

		case Ethernet.gotIP:
			let ip = Net.get("IP", "ethernet");
        	trace(`Ethernet connected. IP address ${ip}\n`);
        	break;

		case Ethernet.disconnected:
			trace(`Ethernet connection lost.\n`);
			break;
    }
});
```

### Example: Get MAC address of Ethernet device

The following example gets the MAC address of the Ethernet device and traces it to the console.

The `Net.get` method is documented in the **Net** section of the [networking documentation](./network.md). Note that there is a second argument passed into the `Net.get` function in this example: the string `"ethernet"`. This specifies that you want to get the MAC address of the ethernet device, not the MAC address of the Wi-Fi device on the ESP32.

```js
let mac = Net.get("MAC", "ethernet");
trace(`Ethernet MAC address is ${mac}\n`);
```
