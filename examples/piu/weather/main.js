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

import {} from "piu/MC";
import { Request } from "http";
import Timeline from "piu/Timeline";
import Timer from "timer";

const APPID = "94de4cda19a2ba07d3fa6450eb80f091";
const country = "us";
const zips = ["94025", "64015", "92014", "93901", "73301"];

const CLOUD = 0;
const SUN = 1;
const SNOW = 2;
const RAIN = 3;
const PARTIAL = 4;
const SUNRAIN = 5;
const TORNADO = 6;
const DARKCLOUD = 7;

const WHITE = "#E7DFDD";
const BLACK = "#202020";

const blackSkin = new Skin({ fill:BLACK });
const iconTexture = new Texture("icons-alpha.png");
const iconSkin = new Skin({ 
	texture: iconTexture, 
	color: [BLACK, WHITE], 
	x: 0, y: 0, width: 150, height: 150, variants: 150, states: 0,
});

const OpenSans52 = new Style({ font: "normal normal normal 52px Open Sans" });
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
	onTimeChanged(column) {
		this.timeline.seekTo(column.time);
	}
	onFinished(column) {
		if (this.transitioningIn) {
			this.transitioningIn = false;
			column.distribute("onStartFlash");	
		} else {
			application.defer("onAddNextScreen");
		}
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
	  	}
		onDisplaying(application) {
			if (application.height != 320 || application.width != 240)
				trace("WARNING: This application was designed to run on a 240x320 screen.\n");
			this.zipIndex = 0;
			this.getWeatherData(application, zips[this.zipIndex]); 
		}
		getWeatherData(application, zip) {
			let request = new Request({
				host: "api.openweathermap.org",
				path: `/data/2.5/weather?zip=${zip},${country}&appid=${APPID}&units=imperial`,
				response: String
			});
			request.callback = (message, value) => {
				if (5 == message) {
					value = JSON.parse(value, ["main", "name", "temp", "weather", "icon"]);
					let icon = value.weather[0].icon.substring(0,2);
					let toDraw;
					switch (icon){
						case "01":
							toDraw = SUN;
							break;
						case "02":
							toDraw = PARTIAL;
							break;
						case "03":
							toDraw = CLOUD;
							break;
						case "04":
							toDraw = DARKCLOUD;
							break;
						case "09":
							toDraw = RAIN;
							break;
						case "10":
							toDraw = SUNRAIN;
							break;
						case "11":
							toDraw = TORNADO;
							break;
						case "13":
							toDraw = SNOW;
							break;
						default:
							toDraw = TORNADO;
							break;
					}
					this.data.city = value.name,
					this.data.temp = Math.round(value.main.temp) + " F",
					this.data.condition = titleCase(value.weather[0].main),
					this.data.icon = toDraw
					application.first.delegate("onTransitionOut");
					Timer.set(() => {
						this.getNext(application);
					}, 5000);
				}
			}
		}
		getNext(application) {
			if (++this.zipIndex >= zips.length) this.zipIndex = 0;
			let zip = zips[this.zipIndex];
			this.getWeatherData(application, zip);     
		}
		onAddNextScreen(application) {
			application.empty();
			application.add(new MainCol(this.data));
		}
}

const WeatherApp = Application.template($ => ({
	left: 0, right: 0, top: 0, bottom: 0, skin: blackSkin, 
	contents: [
		Label($, {
			left: 0, right: 0, top: 0, bottom: 0, 
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