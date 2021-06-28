/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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

import BLEClient from "bleclient";
import BLEServer from "bleserver";
import {uuid} from "btutils";
import GAP from "gap";

class CTSAuthenticator extends BLEServer {
	constructor(client) {
		super();
		this.client = client;
	}
	onReady() {
		this.deviceName = "Moddable Zero";
		this.securityParameters = { mitm:true };
		this.onDisconnected();
	}
	onConnected(device) {
		this.device = device;
		this.stopAdvertising();
	}
	onAuthenticated() {
		this.client.onAuthenticated(this.device);
	}
	onDisconnected() {
		this.startAdvertising({
			advertisingData: {flags: 6, completeName: this.deviceName, solicitationUUID16List: [uuid`1805`]}
		});
	}
}
Object.freeze(CTSAuthenticator.prototype);

class CTSClient extends BLEClient {
	constructor(client, device) {
		super();
		this.client = client;
		this.device = device;
	}
	onReady() {
		this.CURRENT_TIME_SERVICE_UUID = uuid`1805`;
		this.CURRENT_TIME_CHARACTERISTIC_UUID = uuid`2A2B`;
		this.APPLE_IDENTIFIER = 0x004C;
		this.securityParameters = { mitm:true };
	}
	onSecurityParameters() {
		this.connect(this.device);
	}
	onConnected(device) {
		device.discoverPrimaryService(this.CURRENT_TIME_SERVICE_UUID);
	}
	onServices(services) {
		let time_service = services.find(service => service.uuid.equals(this.CURRENT_TIME_SERVICE_UUID));
		if (time_service)
			time_service.discoverCharacteristic(this.CURRENT_TIME_CHARACTERISTIC_UUID);
	}
	onCharacteristics(characteristics) {
		if (characteristics.length)
			characteristics[0].readValue();
	}
	onCharacteristicValue(characteristic, value) {
		let current_date_time = value;
		let year = (current_date_time[1] << 8) | current_date_time[0];
		let month = current_date_time[2];
		let day = current_date_time[3];
		let hour = current_date_time[4];
		let minute = current_date_time[5];
		let second = current_date_time[6];
		let day_of_week = current_date_time[7];
		
		let date = new Date(year, month - 1, day, hour, minute, second);
		this.client.onTimeRead(Math.round(date/1000.0));
	}
}
Object.freeze(CTSClient.prototype);

export {CTSAuthenticator, CTSClient};
