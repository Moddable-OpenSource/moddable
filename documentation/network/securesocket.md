# SecureSocket
Copyright 2017 Moddable Tech, Inc.

Revised: May 10, 2017

Warning: These notes are preliminary. Omissions and errors are likely. If you encounter problems, please ask for assistance.

## Introduction

SecureSocket is an implementation of TLS for the Moddable SDK. It is an evolution of the SecureSocket for Kinoma Element with significant changes to drastically reduce RAM requirements. The essential function and operation remain the same.

At this time, there is no Listener in SecureSocket, only the Socket, which is TCP-only (no UDP).

### Use

SecureSocket implements the same API as the Socket class. See the documentation of the Socket class for details.

SecureSocket extends the dictionary of the constructor with an additional property named "secure" which is itself a dictionary used to configure the TLS connection.

In the following example, the TLS socket is created with support for version 0x303 of TLS, which corresponds to TLS 1.2.

	const host = "www.example.com";
	const port = 443;
	
	let socket = new SecureSocket({host, port,
			secure: {protocolVersion: 0x303}});

### Dictionary

The "secure" dictionary may contain the following properties:

- `protocolVersion` - the minimum version of the TLS protocol to implement. TLS 1.0 is used by default.
- `certificate` - a certificate in DER (binary) format contained in an `ArrayBuffer`, `Uint8Array`, or host buffer
- `trace`  - if true, the TLS stack outputs a trace of its activity. This can be useful in diagnosing failures.
- `verify` - if false, the certificate chain provided by the server is not verified. This should never be done in production systems but can be useful when debugging.
- `tls_max_fragment_length` - a number indicating the requested maximum fragment size. Unfortunately, many servers ignore this optional extension. When supported, can help reduce memory requirements.
- `tls_server_name` - TBD
- `tls_signature_algorithms` - TBD
- `tls_application_layer_protocol_negotiation` - TBD
 

### HTTPS

The HTTP client accepts an optional Socket property in the dictionary of its constructor. Set this to SecureSocket to make an HTTPS request. 

	import SecureSocket from "securesocket";
	
	let request = new Request({host: "www.howsmyssl.com", path: "/",
		response: String, Socket: SecureSocket, port: 443});

The secure property may also be provided:

	let request = new Request({host: "www.howsmyssl.com", path: "/",
		response: String, Socket: SecureSocket, port: 443,
		secure: {trace: true, protocolVersion: 0x303});

### WSS

The WebSockets client accepts an optional Socket property in the dictionary of its constructor. Set this to SecureSocket to make an WSS request. 

	import {Client} from "websocket"
	import SecureSocket from "securesocket";

	let ws = new Client({host: "echo.websocket.org", port: 443,
		Socket: SecureSocket, secure: {protocolVersion: 0x302}});

### Certificates

Work on built-in certificates is on-going. Kinoma Element carries about 300 KB of built-in certificates to be able to connect to a majority of the internet web sites. Many devices, the ESP8266 of note, do not have enough free flash to hold these in the current implementation.

The certificate store is located at ${MODDABLE}/modules/crypt/data. The content of this directory must be pruned before building or the link will fail. Only `ca.ski` is required. This is the index of certificates. All others can be removed. Of course, with no certificates no web sites will work with verification enabled.

For testing, I leave only ca9.der, ca17.der, ca56.der, ca107.der, ca109.der, ca118.der, and ca220.der. To determine which certificate is required for a given web site, remove all certificates and the try to access the web site and see what certificate fails to load.

As an alternative to the certificate store, you can put the certificates needed in your application and pass the appropriate certificate in the "secure" dictionary.

	let request = new Request({host: "www.howsmyssl.com", path: "/",
			response: String, Socket: SecureSocket, port: 443,
			secure: {certificate: new Resource("ca109.der")}});

### Memory

The TLS handshake require a fair amount of memory. The exact amount required varies depending on the site you are connecting to.  As a rough guideline, the following should be free:

- 4 KB of stack
- 10 KB in the chunk heap
- 6 KB in the slot heap
- 2 KB in the system heap

Once the handshake is complete (e.g. once the secure connection is established), memory use drops considerably.

However, if the server sends large fragments (e.g. apple.com sends 16 KB fragments), there is not enough free RAM on the ESP8266 to buffer them. The requests will fail. Secure web servers designed to work with IoT devices will use smaller fragments by default and/or will support the tls_max_fragment_length extension.

When working with HTTPS, it is best to use streaming mode to retrieve the response body as it arrives rather than having the http client buffer the entire response body in memory.

### macOS

SecureSocket works on macOS. The code is the same. macOS is more generous with memory, so requests may succeed on macOS that fail on an MCU.

### To do

Large fragments (e.g. larger than can be fully buffered in RAM) may be possible to support, but it will be complicated.

GCM mode is untested.

Much more testing.

Sort out how to handle built-in certificates.

Listener.
