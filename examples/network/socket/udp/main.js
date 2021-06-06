/*
* Copyright (c) 2021  Moddable Tech, Inc.
*
*   This file is part of the Moddable SDK.
*
*   This work is licensed under the
*       Creative Commons Attribution 4.0 International License.
*   To view a copy of this license, visit
*       <http://creativecommons.org/licenses/by/4.0>.
*   or send a letter to Creative Commons, PO Box 1866,
*   Mountain View, CA 94042, USA.
*
*/

/*

	This example shows how to send and receive UDP packets.
	It requires a UDP echo server. A simple UDP echo server is included in this
	directory in reply-to-empty-udp.py. To run it, use:

		python ./reply-to-empty-udp.py

	Update the address in EchoServer below to the IP address of your echo server.
	Then build and run this example.

	Note: Some Wi-Fi networks are configured to prevent data transmission between
	devices on the same network. If your network is configured in this way,
	this example will not work.

*/

import {Socket} from "socket";
import Timer from "timer"

const EchoServer = {
	address: "10.0.1.8",		// update this for your echo server
	port: 31337
};

const socket = new Socket({kind: "UDP"});
socket.callback = function(message, value) {
	if (Socket.readable !== message)
		return;

	const buffer = this.read(ArrayBuffer);
	const longs = new Uint32Array(buffer);
	trace(`Received ${longs[0]}\n`);
}

let count = 0;
Timer.repeat(() => {
	const longs = Uint32Array.of(count++);
	socket.write(EchoServer.address, EchoServer.port, longs.buffer);
}, 100)
