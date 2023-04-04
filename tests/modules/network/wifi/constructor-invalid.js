/*---
description: 
flags: [module]
---*/

import WiFi from "wifi";
import Timer from "timer";

if (1 !== WiFi.mode) {
	WiFi.mode = 1;
	Timer.delay(3000);
}
WiFi.disconnect();
const options = await $NETWORK.wifi();

assert.throws(Error, () => {new WiFi("123", () => {});}, "options not object");

assert.throws(Error, () => {
	new WiFi({
	});
}, () => {}, "ssid missing");

assert.throws(Error, () => {
	new WiFi({
		ssid: "0".repeat(33)
	});
}, () => {}, "ssid too long");

assert.throws(Error, () => {
	new WiFi({
		ssid: options.ssid,
		password: "0".repeat(65)
	});
}, () => {}, "password too long");

assert.throws(TypeError, () => {
	new WiFi({
		ssid: options.ssid,
		bssid: 1
	});
}, () => {}, "invalid bssid");

assert.throws(Error, () => {
	new WiFi({
		ssid: options.ssid,
		channel: 14
	});
}, () => {}, "invalid channel");

assert.throws(Error, () => {
	new WiFi({
		ssid: options.ssid,
		channel: 0
	});
}, () => {}, "invalid channel");

assert.throws(Error, () => {
	new WiFi({
		ssid: options.ssid,
		channel: {}
	});
}, () => {}, "invalid channel");

