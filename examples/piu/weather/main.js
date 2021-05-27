/*
 * Copyright (c) 2016-2019  Moddable Tech, Inc.
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

import {} from "piu/MC";
import { Request } from "http";
import Timeline from "piu/Timeline";

const APPID = "94de4cda19a2ba07d3fa6450eb80f091";
const country = "us";
const zips = ["95366", "94025", "64015", "92014", "93901", "73301"];
const icons = ["03", "01", "13", "09", "02", "10", "11", "04"];

const WHITE = "#E7DFDD";
const BLACK = "#202020";

const blackSkin = new Skin({ fill:BLACK });
const iconTexture = new Texture("icons-alpha.png");
const iconSkin = new Skin({ 
	texture: iconTexture, 
	color: [BLACK, WHITE], 
	x: 0, y: 0, width: 150, height: 150, variants: 150
});

const OpenSans52 = new Style({ font: "52px Open Sans" });
const OpenSans28 = new Style({ font: "semibold 28px Open Sans", color: WHITE });

function titleCase(str) {
	return str.split(' ').map(i => i[0].toUpperCase() + i.substring(1)).join(' ')
}

class AnimatedTextBehavior extends Behavior {
	onCreate(port, data) {
		this.string = data.string;
		this.style = data.style;
		this.color = data.color;
		this.hue = data.hue;
	}
	onDisplaying(port) {
		this.labelSize = port.measureString(this.string, this.style);
		this.leftMargin = (port.width - this.labelSize.width)/2;
		port.interval = 50;
		port.start();
	}
	onDraw(port, x, y, w, h) {
		let color = hsl(this.hue, 1, 0.75);
		port.drawString(this.string, this.style, color, this.leftMargin, 0, this.labelSize.width, port.height);
		this.hue++;
	}
	onTimeChanged(port) {
		port.invalidate();
	}
};

const AnimatedText = Port.template($ => ({
	width:240,
	Behavior: AnimatedTextBehavior
}));

class MainColBehavior extends Behavior {
	onCreate(content, data) {
		this.data = data;
		this.transitioningIn = 1;
	}
	onDisplaying(column) {
		this.onTransitionIn(column);
	}
	onTimeChanged(column) {
		this.timeline.seekTo(column.time);
	}
	onFinished(column) {
		if (this.transitioningIn)
			this.transitioningIn = false;
		else
			application.defer("onAddNextScreen");
	}
	onTransitionIn(column) {
		this.timeline = (new Timeline)
			.to(column.content("city"), { x:0 }, 750, Math.backEaseOut, 0)
			.to(column.content("temp"), { x:0 }, 750, Math.backEaseOut, -500)
			.from(column.content("condition"), { y: 320 }, 400, Math.quadEaseOut, -750)
			.to(column.content("icon"), { state: 1 }, 500, Math.quadEaseOut, -250)
		column.duration = this.timeline.duration;
		column.time = 0;
		this.timeline.seekTo(0);
		column.start();
	}
	onTransitionOut(column) {
		this.timeline = (new Timeline)
			.to(column.content("icon"), { state: 0 }, 400, Math.quadEaseOut, 0)
			.to(column.content("city"), { x:-240 }, 400, Math.backEaseOut, -200)
			.to(column.content("condition"), { y:320 }, 400, Math.quadEaseOut, -400)
			.to(column.content("temp"), { x:240 }, 400, Math.backEaseOut, -280)
		column.duration = this.timeline.duration;
		column.time = 0;
		this.timeline.seekTo(0);
		column.start();
	}
}

const MainCol = Column.template($ => ({
	left:0, top:0, right: 0, bottom: 0, skin: blackSkin,
	contents:[
		new AnimatedText({ hue: 0, string: $.city, style: OpenSans28, }, { name: "city", top: 15, height: 38, left: 240,  }),
		new AnimatedText({ hue: 90, string: $.temp, style: OpenSans52, }, { name: "temp", top: -4, height: 58, left: -240 }),
		Content($, { name: "icon", width: 150, top: 10, height:150, skin: iconSkin, variant: $.icon }),
		new AnimatedText({ hue: 270, string: $.condition, style: OpenSans28, }, { name: "condition", top: 3, height: 38, }),	
	],
	Behavior: MainColBehavior
}));

class WeatherAppBehavior extends Behavior {
	onCreate(application, data) {
		this.data = data;
		application.duration = 5000;
	}
	onDisplaying(application) {
		if (application.height != 320 || application.width != 240)
			trace("WARNING: This application was designed to run on a 240x320 screen.\n");
		this.zipIndex = 0;
		this.getWeatherData(application, zips[this.zipIndex]); 
	}
	onFinished(application) {
		if (++this.zipIndex >= zips.length) this.zipIndex = 0;
		let zip = zips[this.zipIndex];
		this.getWeatherData(application, zip);     
	}
	getWeatherData(application, zip) {
		let request = new Request({
			host: "api.openweathermap.org",
			path: `/data/2.5/weather?zip=${zip},${country}&appid=${APPID}&units=imperial`,
			response: String
		});
		request.callback = (message, value) => {
			if (Request.responseComplete == message) {
				value = JSON.parse(value, ["main", "name", "temp", "weather", "icon"]);
				let iconID = value.weather[0].icon.substring(0,2);
				let icon = icons.findIndex(element => element == iconID);
				if (-1 == icon)
					icon = 1;	// clear
				this.data.city = value.name,
				this.data.temp = Math.round(value.main.temp) + " F",
				this.data.condition = titleCase(value.weather[0].main),
				this.data.icon = icon;
				application.first.delegate("onTransitionOut");
				application.time = 0;
				application.start();
			}
			else if (Request.error == message) {
				application.first.string = "HTTP request failed";
			}
		}
	}
	onAddNextScreen(application) {
		application.empty();
		application.add(new MainCol(this.data));
	}
}

const WeatherApp = Application.template($ => ({
	left: 0, right: 0, top: 0, bottom: 0, skin: blackSkin, 
	contents: [
		Text($, {
			left: 20, right: 20, top: 100,
			skin: blackSkin, style: OpenSans28, string: "Loading...",
			Behavior: class extends Behavior {
				onTransitionOut(label) {
					application.defer("onAddNextScreen");
				}
			}
		}),
	],
	Behavior: WeatherAppBehavior
}));

export default new WeatherApp({});
