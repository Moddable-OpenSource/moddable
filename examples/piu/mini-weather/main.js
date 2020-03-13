/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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
const zips = ["94025", "64015", "92014", "93901", "73301"];

const CLOUD = 0;
const SUN = 1;
const SNOW = 2;
const RAIN = 3;
const PARTIAL = 4;
const SUNRAIN = 5;
const TORNADO = 6;
const DARKCLOUD = 7;

const WHITE = "#FFFFFF";
const BLACK = "#000000";
const CYAN = "#00FFFF";
const GREEN = "#00FF00";
const BLUE = "#0000FF";

const blackSkin = new Skin({ fill:BLACK });
const iconTexture = new Texture("icons-alpha.png");
const iconSkin = new Skin({ texture:iconTexture, x: 0, y: 0, width: 120, height: 120, variants: 120, color: WHITE, aspect:"fit" });

const OpenSans52 = new Style({ font: "normal normal normal 52px Open Sans", color: GREEN, horizontal:"center", vertical:"middle" });
const OpenSans20Cyan = new Style({ font: "semibold 20px Open Sans", color: CYAN, horizontal:"center", vertical:"middle" });
const OpenSans20White = new Style({ font: "semibold 20px Open Sans", color: WHITE, horizontal:"center", vertical:"middle" });
const OpenSans28 = new Style({ font: "semibold 28px Open Sans", color: BLUE, horizontal:"center", vertical:"middle" });

function titleCase(str) {
  return str.split(' ').map(i => i[0].toUpperCase() + i.substring(1)).join(' ')
}

let MainCol = Column.template($ => ({
	left:0, top:0, right: 0, bottom: 0, 
	skin: blackSkin, clip: true, 
	contents:[
		Label($, {anchor: "city", left:240, width: 176, top:3, height:30, string:$.city, style: OpenSans20Cyan}),
		Container($, {
			left:0, right:0, top: 0, bottom: 0, 
			contents:[
				Label($, {anchor: "temp", left:240, width: 176, top: 0, bottom: 20, string:$.temp, style: OpenSans52}),
				Content($, {anchor: "icon", left:285, width: 120, top: 0, height:120, skin: iconSkin, variant: $.icon}),	
				Label($, {anchor: "condition", left:240, width: 176, bottom: 1, height:22, string:$.condition, style: OpenSans20White}),
			]
		}),
	],
	Behavior: class extends Behavior {
		onCreate(content, data) {
			this.data = data;
		}
		onDisplaying(column) {
			let data = this.data;
			let duration = 6360;
			this.timeline = (new Timeline)
				.on(data["city"], { x: [176, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -176] }, duration, Math.linearEase, 0)
				.on(data["temp"], { x: [176, 0, 0, 0, -176] }, 530*5, Math.quadEaseInOut, -duration+530*2)
				.on(data["icon"], { x: [176+28, 28, 28, 28, -176-28] }, 530*5, Math.quadEaseInOut, -duration+530*6)
				.on(data["condition"], { x: [176, 0, 0, 0, -176] }, 530*5, Math.quadEaseInOut, -duration+530*6)
			column.duration = this.timeline.duration;
			column.time = 0;
			column.start();
		}
		onTimeChanged(column) {
			this.timeline.seekTo(column.time);
		}
		onFinished(column) {
	  		column.stop();
		}
		onTransitionOut(label) {
		  application.defer("onAddNextScreen");
		}
	}
}));

const WeatherApp = Application.template($ => ({
	contents: [
		Text($, {
			left: 15, right: 15, top: 60, 
			skin: blackSkin, style: OpenSans20White, string: "Loading...",
			Behavior: class extends Behavior {
				onTransitionOut(label) {
					application.defer("onAddNextScreen");
				}
			}
		}),
	],
	Behavior: class extends Behavior {
	  	onCreate(application, data) {
			this.data = data;
			this.zipIndex = 0;
			let zip = zips[this.zipIndex];
			this.getWeatherData(application, zip); 
			application.interval = 6500;
			application.start();
		}
		onDisplaying(application) {
			if (application.height != 176 || application.width != 176)
				trace("WARNING: This application was designed to run on a 176x176 screen.\n");
		}
		onTimeChanged(application) {
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
					value = JSON.parse(value);
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
					this.data.temp = value.main.temp + " F",
					this.data.condition = titleCase(value.weather[0].description),
					this.data.icon = toDraw
					application.first.delegate("onTransitionOut");
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
}));

export default new WeatherApp({});
