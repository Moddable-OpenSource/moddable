/*
 * Copyright (c) 2016-2021 Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import {} from "piu/MC";
import Time from "time";
import { getTimeEstimate } from "directions";

const WHITE = "white";
const BLACK = "black";

const backgroundSkin = new Skin({ fill: [WHITE, BLACK] });

const arrowTexture = new Texture({ path: "arrow.png" });
const arrowSkin = new Skin({
	texture: arrowTexture,
	height: 28, width: 30, variants: 30
})

const openSans35 = new Style({ font: "35px Open Sans", color: [BLACK, WHITE] });
const openSans52 = new Style({ font: "52px Open Sans", color: [BLACK, WHITE] });
const openSans72 = new Style({ font: "72px Open Sans", color: [BLACK, WHITE] });


let dayStrings = ["Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"];
let monthStrings = ["January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"];

function getDateString(d) {
	return `${dayStrings[d.getDay()]}, ${monthStrings[d.getMonth()]} ${d.getDate()}`
}

function getTimeString(d) {
	let ampm = "AM";
	let hours = d.getHours();
	if (hours == 0)
		hours = 12;
	if (hours > 12) {
		hours -= 12;
	  ampm = "PM";
	}
	let minutes = d.getMinutes();
	if (minutes < 10)
		minutes = "0" + minutes.toString(10);
	return `${hours}:${minutes} ${ampm}`;
}

class ArrowBehavior extends Behavior {
	onTouchBegan(arrow) {
		arrow.variant = !arrow.variant;
		application.delegate("swapDirections");
	}
}

class AppBehavior extends Behavior {
	onCreate(application, data) {
		this.data = data;
		this.direction = false;
	}
	onDisplaying(application) {
		screen.configure({updateMode: "GC16"});
		let humidityTemperature = this.humidityTemperature = new device.sensor.HumidityTemperature;
		this.update(application);
		this.getTravelTime(application);
		application.interval = 10000;
		application.start();
	}
	onTimeChanged(application) {
		this.update(application);
		let now = Time.ticks;
		if (this.lastTravelUpdate - now >= 1000 * 60 * 10)	// Update travel time estimate every 10 minutes
			this.getTravelTime(application);
	}
	update(application) {
		let data = this.data;
		let sample = this.humidityTemperature.sample();
		let c = Math.round(sample.thermometer.temperature);
		let str = `${Math.round((c*9/5) + 32)}°F / ${c}°C`;
		if (data["TEMPERATURE"].string != str) data["TEMPERATURE"].string = str;
		let d = new Date();
		str = getDateString(d);
		if (data["DATE"].string != str) data["DATE"].string = str;
		str = getTimeString(d);
		if (data["TIME"].string != str) data["TIME"].string = str;
	}
	getTravelTime(application) {
		getTimeEstimate(this.direction);
	}
	swapDirections(application) {
		this.direction = !this.direction;
		this.getTravelTime(application);
	}
	onTravelTimeUpdate(application, time) {
		this.lastTravelUpdate = Time.ticks;
		let data = this.data;
		if (time === null) {
			data["TRAVEL_TIME"].style = openSans35;
			data["TRAVEL_TIME"].string = "No route found";
		} if (typeof time === "string") {
			data["TRAVEL_TIME"].style = openSans35;
			data["TRAVEL_TIME"].string = time;
		} else {
			let durationInTraffic = time[0];
			// let durationInTrafficValue = time[1]; // Could use this to show ETA
			let duration = time[2];
			data["TRAVEL_TIME"].style = openSans72;
			data["TRAVEL_TIME"].string = durationInTraffic;
			let diff = durationInTraffic/duration;
			if (diff >= 1.4)
				data["TRAFFIC"].string = "Traffic is heavy";
			else if (diff >= 1.25)
				data["TRAFFIC"].string = "Traffic is light";
			else
				data["TRAFFIC"].string = "No traffic";
		}
	}

}

const M5PaperApp = Application.template($ => ({
	displayListLength: 5000,
	left: 0, right: 0, top: 0, bottom: 0, skin: backgroundSkin,
	contents: [
		Column($, {
			top: 0, bottom: 0, left: 0, width: 450, skin: backgroundSkin, state: 1,
			contents: [
				Content($, { top: 0, bottom: 0 }),
				Label($, {
					anchor: "DATE", top: 0, left: 30, style: openSans35, state: 1
				}),
				Label($, {
					anchor: "TIME", top: 0, left: 30, style: openSans72, state: 1
				}),
				Label($, {
					anchor: "TEMPERATURE", top: 5, left: 30, style: openSans35, state: 1
				}),
				Content($, { top: 0, bottom: 0 })
			]
		}),
		Column($, {
			left: 450, right: 0,
			contents: [
				Row($, {
					contents: [
						Label($, {
							anchor: "HOME", top: 0, left: 0, 
							skin: backgroundSkin, style: openSans35, state: 1, string: "  Home  "
						}),
						Content($, {
							left: 10, right: 10, skin: arrowSkin,
							active: true, Behavior: ArrowBehavior
						}),
						Label($, {
							anchor: "WORK", top: 0, left: 0, 
							skin: backgroundSkin, style: openSans35, state: 1, string: "  Work  "
						})
					]
				}),
				Text($, {
					anchor: "TRAVEL_TIME", top: 20, left: 30, right: 30, style: openSans72
				}),
				Label($, {
					anchor: "TRAFFIC", top: 15, style: openSans35
				}),
			]
		})
	],
	Behavior: AppBehavior
}));

export default new M5PaperApp({});
