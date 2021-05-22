# Giphy

This application allows you to download and display animated GIFs from [Giphy](https://www.giphy.com) on a [Moddable Two](./../../documentation/devices/moddable-two.md).

## Installation instructions

1. Follow the [instructions](https://support.giphy.com/hc/en-us/articles/360020283431-Request-A-GIPHY-API-Key) by Giphy to create an API key

2. Open `manifest.json`. In line 5, change `YOUR_API_KEY_HERE` to your API key.

	```
	"apiKey": "YOUR_API_KEY_HERE"
	```
	
	In lines 3 and 4, change `YOUR_SSID_HERE` and `YOUR_PASSWORD_HERE` to your Wi-Fi network credentials.
	
	```
	"ssid": "YOUR_SSID_HERE",
	"password": "YOUR_PASSWORD_HERE",
	```

3. Use `mcconfig` to build and install the example on Moddable Two.

	```text
	cd $MODDABLE/contributed/giphy
	mcconfig -d -m -p esp32/moddable_two
	```