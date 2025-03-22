/*
 * Copyright (c) 2024 Moddable Tech, Inc.
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

import SLIP from "./slip.js"

const script = `
function* user() {
  guess("9876543210");
  yield;
}
`;

const nordic_uart = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
const received = [];
let device = null;
let server = null;
let rxCharacteristic = null;
const encoder = new TextEncoder;
const decoder = new TextDecoder;

function wait(ms)
{
	return new Promise(resolve => setTimeout(resolve, ms));
}

async function attack(script) {
	const buffer = SLIP.escape(encoder.encode(script));
	const packetSize = 22;		//@@ should be bigger.... MTU size would be nice....
	for (let i = 0; i < buffer.byteLength; i += packetSize) {
		const use = Math.min(packetSize, buffer.byteLength - i);
		await rxCharacteristic.writeValue(buffer.slice(i, i + use));
	}
}

function gattserverdisconnected(event) {
	log(`GATT server disconnected!`);
	document.querySelector("#connect").hidden = false;
	document.querySelector("#disconnect").hidden = true;
	document.querySelector("#attack").hidden = true;
	device.removeEventListener("gattserverdisconnected", gattserverdisconnected);
}

async function connect(options) {
	if (!navigator.bluetooth)
		return log('Web Bluetooth not available in this browser!');
		
	try {
		log('Requesting Bluetooth devices with Nordic UART service...');
		device = await navigator.bluetooth.requestDevice({
			filters: [
				{
					services: [nordic_uart]
				}
			],
			optionalServices: [
				"device_information",
				"battery_service"
			]
		});
		
		log('Connecting to GATT Server...');
		let terminate = false;
		server = await device.gatt.connect();
		device.addEventListener("gattserverdisconnected", gattserverdisconnected);
		
		log(`Setting up serial connection...`);
		const uartService = await server.getPrimaryService(nordic_uart);
		rxCharacteristic = await uartService.getCharacteristic("6e400002-b5a3-f393-e0a9-e50e24dcca9e");	// remote rx
		const txCharacteristic = await uartService.getCharacteristic("6e400003-b5a3-f393-e0a9-e50e24dcca9e");	// remote tx
		await txCharacteristic.startNotifications();
		
		log("Ready!");
		document.querySelector("#connect").hidden = true;
		document.querySelector("#disconnect").hidden = false;
		document.querySelector("#attack").hidden = false;

		const received = new Uint8Array(new ArrayBuffer(0, {maxByteLength: 16384}));
		txCharacteristic.addEventListener("characteristicvaluechanged", function(event) {
			const byteLength = received.byteLength;
			received.buffer.resize(received.byteLength + event.target.value.buffer.byteLength);
			received.set(new Uint8Array(event.target.value.buffer), byteLength); 

			const data = SLIP.unescape(received.buffer);
			if (data.packet)
				log(decoder.decode(data.packet));
			received.buffer.resize(data.remainder?.byteLength ?? 0);
			if (data.remainder)
				received.set(new Uint8Array(data.remainder)); 
		});
	}
	catch (error) {
		log(`${error}`);
		server?.disconnect();
		server = null;
	}
}

let previousText = "";
let logLines = 0;
function log(msg, replace = false) {
	const log = document.getElementById("log");
	if (!replace) {
		previousText = log.value;
		if (++logLines > 1000) {
			logLines = 950;
			previousText = previousText.split("\n");
			previousText = previousText.slice(-logLines);
			previousText = previousText.join("\n");
		}
	}
	log.value = previousText + msg + "\n";
	
	log.scrollTop = log.scrollHeight;
}

document.querySelector("#connect").addEventListener("click", () => {
	connect();
});
document.querySelector("#disconnect").addEventListener("click", () => {
	server?.disconnect();
	server = null;
});
document.querySelector("#attack").addEventListener("click", () => {
	attack(document.getElementById("script").value);
});
