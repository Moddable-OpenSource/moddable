/*
 * Copyright (c) 2025  Moddable Tech, Inc.
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

import { GATTServer} from "embedded:io/bluetoothle/peripheral"
import Timer from "timer";

const deviceName = "419 HRM";

new GATTServer({
	mtu: 128,
	services: [
		// Generic Access Service
		{
			uuid: "1800",
			characteristics: [
				{
					uuid: "2a00",	// device name
					properties: GATTServer.properties.read, 
					value: ArrayBuffer.fromString(deviceName),
				},
				{
					uuid: "2a01",	// appearance
					properties: GATTServer.properties.read, 
					value: Uint8Array.of(0x40, 0x03),
				},
			]
		},
		// Battery Service
		{
			uuid: "180f",
			characteristics: [
				{
					uuid: "2a19",		// // battery level
					properties: GATTServer.properties.read | GATTServer.properties.write,
					onRead(/* connection */) {
						return Uint8Array.of(this.value ?? 50);		// last value written or default of 50%
					},
					onWrite(buffer, connection) {			// write support for demonstration purposes
						this.value = (new Uint8Array(buffer))[0];
						if (0 === this.value)
							connection.close();		// out of battery!
					},
					descriptors: [
						{
							uuid: "2901",		// User description
							value: ArrayBuffer.fromString("Battery level")
						},
						{
							uuid: "2904",		// Characteristic presentation format
							value: Uint8Array.of(4, 0, 0xad, 0x27, 1, 0, 0)
						},
						{
							uuid: "2999",		// test
							onRead() {
								return Uint8Array.of(this.value ?? 128);
							},
							onWrite(buffer) {
								this.value = (new Uint8Array(buffer))[0];
							}
						}
					]
				}				
			]
		},
		// Heart Rate Monitor Service
		{
			uuid: "180d",
			characteristics: [
				{
					uuid: "2a37",		// heart rate
					properties: GATTServer.properties.read | GATTServer.properties.indicate,
					onRead() {
						return Uint8Array.of(0, 65);	// 65 BPM
					},
					onSubscribe(connection) {
						connection.heartRateTimer ??= Timer.set(() => {
							connection.notify(this, Uint8Array.of(0, 55 + Math.random() * 10), error => trace(`notify complete ${error}\n`));
						}, 1, connection.heartRateNotificationInterval * 1000);
					},
					onUnsubscribe(connection) {
						Timer.clear(connection.heartRateTimer);
						delete connection.heartRateTimer;
					}
				}
			]
		},
		// Heart Rate notification frequency – custom service and characteristic
		{
			uuid: "cfe6f7f2-16d2-4b7e-9253-a1d1c1577a35",
			characteristics: [
				{
					uuid: "7d3a0f66-19a9-4fc4-b3e6-91a2d2c2e58a",
					properties: GATTServer.properties.read | GATTServer.properties.write,
					onRead(connection) {
						return Uint8Array.of(connection.heartRateNotificationInterval);
					},
					onWrite(buffer, connection) {
						const value = (new Uint8Array(buffer))[0];
						if (value > 0) {
							connection.heartRateNotificationInterval = value;
							if (connection.heartRateTimer)
								Timer.schedule(connection.heartRateTimer, value * 1000, value * 1000);
						}
					},
					descriptors: [
						{
							uuid: "2901",		// User description
							value: ArrayBuffer.fromString("Update frequency in seconds")
						}
					]
				}
			]
		}
	],
	onConnect(connection) {
		trace("connect!\n");
		connection.heartRateNotificationInterval = 1;
	},
	onDisconnect(connection) {
		trace("disconnect!\n");
		Timer.clear(connection.heartRateTimer);
	},
	onReady() {
		trace("hrm onReady\n");
		this.startAdvertising({
			flags: 6,
			name: deviceName,
			manufacturerData: {
				manufacturer: 1,
				data: Uint8Array.of(255, 0, 1)
			},
			services: ["180d", "180f"],
			[123]: Uint8Array.of(1,2,3)
		});
	},
	onWarning(msg) {
		trace(`BLE server warning: ${msg}\n`);
	}
});
