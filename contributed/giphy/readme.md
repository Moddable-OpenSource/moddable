# Giphy

This application allows you to download and display animated GIFs from [Giphy](https://www.giphy.com) on a [Moddable Two](./../../documentation/devices/moddable-two.md).

## Installation instructions

1. Follow the [instructions](https://support.giphy.com/hc/en-us/articles/360020283431-Request-A-GIPHY-API-Key) by Giphy to create an API key

2. Open `main.js`. In line 22, change `YOUR_API_KEY_HERE` to your API key

3. Use `mcconfig` to build and install the example on Moddable Two. Be sure to specify your Wi-Fi credentials:

	```text
	cd $MODDABLE/contributed/giphy
	mcconfig -d -m -p esp32/moddable_two ssid="YOUR_SSID_HERE" password="YOUR_PASSWORD_HERE"
	```