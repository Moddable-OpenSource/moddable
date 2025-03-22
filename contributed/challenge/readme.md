# Embedded Challenge
Updated: September 13, 2024

This directory contains an embedded version of the [Challenge](https://hardenedjs.org/challenge/) presented on the [Hardened JavaScript web site](https://hardenedjs.org). The Challenge demonstrates how JavaScript code executing inside a Hardened JavaScript `Compartment` is prevented from accessing secret data, such as passwords, and powerful functions that the host wants to keep to itself.

The Embedded Challenge is designed for [Moddable Six](https://www.moddable.com/moddable-six), can can be easily adapted to run on any host with comparable capabilities (memory, touch screen, BLE). To build and run the challenge on Moddable Six install the Moddable SDK and ESP-IDF, and then:

```
> cd $MODDABLE/contributed/challenge
> mcconfig -d -m -p esp32/moddable_six_cdc
```

The user interface shows the secret code and latest guess. It also allows you to run the example attacker scripts from the Hardened JavaScript web page. You can run your own scripts too.  To do that, you'll use a local web page and BLE to communicate with the Moddable Six.

1. Start-up a web server

	```
	> cd $MODDABLE/contributed/challenge/web
	> python3 -m http.server 8080
	```

2. Launch Chrome (Chrome is currently the only browser that implements Web BLE)
3. Open the web page at `http://localhost:8080`
4. Press the "Connect" button to establish a connection to the embedded device
5. Enter your attack script
6. Press the "Attack" button to run the script on the embedded device
