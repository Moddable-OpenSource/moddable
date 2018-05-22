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
 /*
	https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.service.health_thermometer.xml
	https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.temperature_measurement.xml
 */

import BLEServer from "bleserver";
import Timer from "timer";
import Poco from "commodetto/Poco";
import Resource from "Resource";
import parseBMF from "commodetto/parseBMF";
import parseBMP from "commodetto/parseBMP";
import parseRLE from "commodetto/parseRLE";

class HealthThermometerService extends BLEServer {
	constructor(render) {
		super();
		this.render = render;
		this.white = render.makeColor(255, 255, 255);
		this.blue = render.makeColor(0x08, 0x82, 0xFB);
		this.titleFont = parseBMF(new Resource("OpenSans-Semibold-20.bf4"));
		this.temperatureFont = parseBMF(new Resource("OpenSans-full-Semibold-44.bf4"));
		this.tempIcon = parseRLE(new Resource("temp-alpha.bm4"));
		this.logo = parseBMP(new Resource("Bluetooth_FM_Color-color.bmp"));
		this.logo.alpha = parseBMP(new Resource("Bluetooth_FM_Color-alpha.bmp"));
		this.title = "Health Thermometer";
		this.state = "Advertising...";
		this.temp = 0;
		this.drawBackground();
		this.drawTemperature();
		this.drawFooter();
	}
	onReady() {
		this.timer = null;
		this.deviceName = "Moddable HTM";
		this.onDisconnected();
		this.deploy();
	}
	onConnected() {
		this.stopAdvertising();
	}
	onDisconnected() {
		this.stopMeasurements();
		this.state = "Advertising...";
		this.temp = 0;
		this.drawTemperature();
		this.drawFooter();
		this.startAdvertising({
			advertisingData: {flags: 6, completeName: this.deviceName, completeUUID16List: ["1809","180F"]}
		});
	}
	onCharacteristicNotifyEnabled(characteristic) {
		let render = this.render;
		this.state = "Reading...";
		this.drawFooter();
		this.startMeasurements(characteristic);
	}
	onCharacteristicNotifyDisabled(characteristic) {
		this.stopMeasurements();
	}
	get temperature() {
		if (98.5 > this.temp)
			this.temp += 0.1;
		let flags = 0x01;		// fahrenheit
		let exponent = 0xFD;	// -1
		let mantissa = Math.round(this.temp * 1000);
		let temp = (exponent << 24) | mantissa;		// IEEE-11073 32-bit float
		let result = [flags, temp & 0xFF, (temp >> 8) & 0xFF, (temp >> 16) & 0xFF, (temp >> 24) & 0xFF];
		return result;
	}
	startMeasurements(characteristic) {
		this.temp = 95.0;
		this.timer = Timer.repeat(id => {
			this.notifyValue(characteristic, this.temperature);
			this.drawTemperature();
			if (this.temp > 98.5) {
				this.state = "Done!";
				this.drawFooter();
				this.stopMeasurements();
			}
		}, 250);
	}
	stopMeasurements() {
		if (this.timer) {
			Timer.clear(this.timer);
			this.timer = null;
		}
	}
	drawBackground() {
		let render = this.render;
		let white = this.white;
		let blue = this.blue;
		let font = this.titleFont;
		let tempIcon = this.tempIcon;
		let logo = this.logo;
		render.begin();
			render.fillRectangle(white, 0, 0, render.width, render.height);
			render.fillRectangle(blue, 0, 0, render.width, font.height);
			render.drawText(this.title, font, white,
				(render.width - render.getTextWidth(this.title, font)) >> 1, 0);
			render.drawMasked(logo, (render.width - logo.width) >> 1, render.height - 120, 0, 0, logo.width, logo.height, logo.alpha, 0, 0);
		render.end();
	}
	drawFooter() {
		let render = this.render;
		let white = this.white;
		let blue = this.blue;
		let font = this.titleFont;
		render.begin(0, render.height - font.height, render.width, font.height);
			render.fillRectangle(blue, 0, 0, render.width, render.height);
			render.drawText(this.state, font, white,
				(render.width - render.getTextWidth(this.state, font)) >> 1, render.height - font.height);
		render.end();
	}
	drawTemperature() {
		let text;
		if (this.temp == 0)
			text = "-.-";
		else
			text = this.temp.toFixed(2);
		text += "°F";
		let render = this.render;
		let white = this.white;
		let blue = this.blue;
		let font = this.temperatureFont;
		render.begin(0, 70, render.width, font.height);
			render.fillRectangle(white, 0, 0, render.width, render.height);
			render.drawText(text, font, blue,
				(render.width - render.getTextWidth(text, font)) >> 1, 70);
		render.end();
	}
}

let render = new Poco(screen, { rotation:180 });
let htm = new HealthThermometerService(render);
