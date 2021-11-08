# Giphy

This application allows you to download and display animated GIFs from [Giphy](https://www.giphy.com) on a [Moddable Two](./../../documentation/devices/moddable-two.md).

For more information on the implementation, see the blog post [GIPHY Goes to ESP32](https://blog.moddable.com/blog/giphy/).

## Installation instructions

1. Follow the [instructions](https://support.giphy.com/hc/en-us/articles/360020283431-Request-A-GIPHY-API-Key) by Giphy to create an API key

2. Open `manifest.json`. In lines 3-5, change `YOUR_SSID_HERE`, `YOUR_PASSWORD_HERE`, and `YOUR_API_KEY_HERE` to your Wi-Fi network credentials and Giphy API key.

	```text
	"ssid": "YOUR_SSID_HERE",
	"password": "YOUR_PASSWORD_HERE",
	"apiKey": "YOUR_API_KEY_HERE"
	```

3. Use `mcconfig` to build and install the example on Moddable Two.

	```text
	cd $MODDABLE/contributed/giphy
	mcconfig -d -m -p esp32/moddable_two
	```
	
> **Note:** Instead of putting your Wi-Fi network credentials and Giphy API key in the app manifest, you can also specify them in the `mcconfig` command:
> 
> ```text
> mcconfig -d -m -p esp32/moddable_two ssid="YOUR_SSID_HERE" password="YOUR_PASSWORD_HERE" apiKey="YOUR_API_KEY_HERE"
> ```