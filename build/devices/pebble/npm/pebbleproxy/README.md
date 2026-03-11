# @moddable/pebbleproxy

A PKJS-side HTTP and WebSocket proxy for Pebble OS apps using the Moddable SDK.

This package implements a proxy for JavaScript Pebble OS applications using the Moddable SDK. The proxy supports the following services:

- HTTP requests
- WebSocket connections
- Location

The package expects to run under PKJS, either in the Pebble Mobile app or the Pebble QEMU emulator.

## Installation
Add this package to your Pebble application's package.json in the dependencies section. The Pebble build system automatically installs the package.

```jsonc
"dependencies": {
	"@moddable/pebbleproxy": "^0.1.5"
},
```

## Use
### Initialization and Receiving Messages
To use the Pebble Proxy, you need to include support in your project's PKJS code, usually found at `./src/pkjs/index.js`. If your application doesn't use PKJS for anything else, keep it simple:

```js
const moddableProxy = require("@moddable/pebbleproxy");
Pebble.addEventListener('ready', moddableProxy.readyReceived);
Pebble.addEventListener('appmessage', moddableProxy.appMessageReceived);
```

If your project does use PKJS, here's a good starting point:

```js
const moddableProxy = require("@moddable/pebbleproxy");
Pebble.addEventListener('ready', moddableProxy.readyReceived);
Pebble.addEventListener('appmessage', function (e) {
	if (moddableProxy.appMessageReceived(e))
		return;

	// This is not a proxy message. Handle the message here. 
});
```

> **Note**: `require()` is necessary for compatibility with PKJS.

### Sending Messages
The connection between PKJS and Pebble OS limits the number of messages that can be simultaneously enqueued. If too many messages are sent at about the same time, some may fail. The proxy provides an alternate API for PKJS's `Pebble.sendAppMessage()`. 

```js
moddableProxy.sendAppMessage({hello: "world"});
```

This queues messages to be sent as soon as possible, avoiding failures due to messages being sent too quickly.

### Debug Logging
For debugging, the proxy has a logging feature. It is off by default, but can be easily enabled.

```js
moddableProxy.log = true;
```

## License
Licensed under the GNU Lesser General Public License v3.0 (LGPL-3.0).

Part of the [Moddable SDK](https://github.com/Moddable-OpenSource/moddable) Runtime.
