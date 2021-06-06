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
	This example demonstrates how to synchronize the ESP32 clock with an iPhone over BLE.
	Launch the example and connect to the Moddable Zero from your iPhone Bluetooth settings.
	After synchronizing the time over BLE, the current time and date will be displayed on the LCD.
	
	mcconfig -d -m -p esp32/moddable_zero

	https://www.bluetooth.com/wp-content/uploads/Sitecore-Media-Library/Gatt/Xml/Services/org.bluetooth.service.current_time.xml
*/

import {CTSAuthenticator, CTSClient} from "cts";
import Time from "time";
import Timer from "timer";
import Poco from "commodetto/Poco";
import parseBMF from "commodetto/parseBMF";
import Resource from "Resource";

let render = new Poco(screen, { rotation:270 });
let timeFont = parseBMF(new Resource("OpenSans-Regular-52.bf4"));
let dateFont = parseBMF(new Resource("OpenSans-Regular-20.bf4"));
let blue = render.makeColor(0x19, 0x2e, 0xab);
let white = render.makeColor(255, 255, 255);

class TimeSyncClient {
	start() {
		this.server = new CTSAuthenticator(this);
	}
	onAuthenticated(device) {
		this.client = new CTSClient(this, device);
	}
	onTimeRead(date) {    
		Time.set(date);
		this.displayTime();
		this.server.close();
		this.client.close();
	}
	displayTime() {
		let timer = Timer.repeat(() => {
			let date = new Date();
			let hours = String(date.getHours());
			let minutes = String(date.getMinutes());
			let seconds = String(date.getSeconds());
			let ampm = (hours > 11 ? ' PM' : ' AM');
			if (hours > 12)
				hours -= 12;
			if (1 == minutes.length)
				minutes = '0' + minutes;
			if (1 == seconds.length)
				seconds = '0' + seconds;
			let time = hours + ':' + minutes + ':' + seconds + ampm;
			let timeWidth = render.getTextWidth(time, timeFont);
			let x = (render.width - timeWidth) >> 1;
			let y = ((render.height - timeFont.height) >> 1) - 10;
			render.begin(x, y, timeWidth, timeFont.height);
				render.fillRectangle(blue, 0, 0, render.width, render.height);
				render.drawText(time, timeFont, white, x, y);
			render.end();
			
			date = date.toString().split(' ');
			date.length = 4;
			date = date.join(' ');
			let dateWidth = render.getTextWidth(date, dateFont);
			x = (render.width - dateWidth) >> 1;
			y = (render.height - dateFont.height) - 10;
			render.begin(x, y, dateWidth, dateFont.height);
				render.fillRectangle(blue, 0, 0, render.width, render.height);
				render.drawText(date, dateFont, white, x, y);
			render.end();
		}, 500);
	}
}

render.begin();
	render.fillRectangle(blue, 0, 0, render.width, render.height);
render.end();

let client = new TimeSyncClient;
client.start();
