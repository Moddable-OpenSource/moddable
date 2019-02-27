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

import {
  Application,
  Behavior,
  Column,
  Content,
  Label,
  Port,
  Skin,
  Style,
  Texture, 
  hsl
} from "piu/MC";

import {
  Request
} from "http";

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

/* Skins, styles, & textures */
const WHITE = "#E7DFDD";
const BLACK = "#3b3a36";
const ORANGE = [21, 0.36];	//"#AD7252";
const GREEN = [101, 0.39]; 	//"#6DB14E";
const BLUE = [183, 0.36];	//"#52A9AD";

const blackSkin = new Skin({ fill:BLACK });
const iconTexture = new Texture("icons-alpha.png");
const iconSkin = new Skin({ texture:iconTexture, x: 0, y: 0, width: 150, height: 150, variants: 150, color: WHITE, aspect:"fit" });

const OpenSans52 = new Style({ font: "normal normal normal 52px Open Sans" });
const OpenSans20 = new Style({ font: "semibold 20px Open Sans", color: "#6DB14E" });
const OpenSans28 = new Style({ font: "semibold 28px Open Sans" });

/* UI templates */
const AnimatedText = Port.template($ => ({
	name: $.name, top: $.top, height: $.height, left: 240, width:240,
	Behavior: class extends Behavior {
		onCreate(port, data) {
			this.string = data.string;
			this.style = data.style;
			this.color = data.color;
			this.l = 0.5;
			this.direction = -1;
		}
		onDisplaying(port) {
			this.labelSize = port.measureString(this.string, this.style);
			this.leftMargin = (port.width - this.labelSize.width)/2;
		}
		onStartFlash(port) {
			this.direction = 0;
			port.interval = 75;
			port.start();
		}
		onStopFlash(port) {
			port.stop();
			this.direction = -1;
		}
		onDraw(port) {
			let color = hsl(this.color[0], this.color[1], this.l);
			port.drawString(this.string, this.style, color, this.leftMargin, 0, this.labelSize.width, port.height);
			if ((this.direction == 0) && (this.l > 0.6)){
				this.direction = 1;
			} else if ((this.direction == 1) && (this.l < 0.5)) {
				this.onStopFlash(port);
			}
			if (this.direction == 0) this.l += 0.03;
			else if (this.direction == 1) this.l -= 0.03;
		}
		onTimeChanged(port) {
			if (this.direction > -1) port.invalidate();
		}
	}
}));

let MainCol = Column.template($ => ({
	left:0, top:0, right: 0, bottom: 0, skin: blackSkin, contents:[
		new AnimatedText({ name: "city", top: 18, height: 38, string: $.city, style: OpenSans28, color: GREEN }),
		new AnimatedText({ name: "temp", top: -4, height: 58, string: $.temp, style: OpenSans52, color: BLUE }),
		Content($, {name: "icon", left:285, width: 150, top: 10, height:150, skin: iconSkin, variant: $.icon}),
		new AnimatedText({ name: "condition", top: 3, height: 26, string: $.condition, style: OpenSans20, color: ORANGE }),	
	],
	Behavior: class extends Behavior {
		onCreate(content, data) {
			this.transitioningIn = 1;
		}
		onDisplaying(column) {
			this.timeline = (new Timeline)
				.to(column.first, { x:0 }, 500, Math.quadEaseOut, 0)
				.to(column.first.next, { x:0 }, 500, Math.quadEaseOut, -450)
				.to(column.first.next.next, { x:45 }, 500, Math.quadEaseOut, -450)
				.to(column.last, { x:0 }, 500, Math.quadEaseOut, -450)
			column.duration = this.timeline.duration;
			column.time = 0;
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
				.to(column.first, { x:-240 }, 400, Math.quadEaseIn, 0)
				.to(column.first.next, { x:-240 }, 400, Math.quadEaseIn, -280)
				.to(column.first.next.next, { x:-285 }, 400, Math.quadEaseIn, -280)
				.to(column.last, { x:-240 }, 400, Math.quadEaseIn, -280)
			column.duration = this.timeline.duration;
			column.time = 0;
			column.start();
		}
	}
}));

/* Application set-up */
export default new Application(null, {
	Behavior: class extends Behavior {
	  onCreate(application) {
	  	application.add(Label(null, {
	  		left: 0, right: 0, top: 0, bottom: 0, skin: blackSkin, 
	  		style: OpenSans20, string: "Loading...",
	  		Behavior: class extends Behavior {
	  			onTransitionOut(label) {
	  				application.defer("onAddNextScreen");
	  			}
	  		}
	  	}))
		this.zipIndex = 0;
		let zip = zips[this.zipIndex];
		this.getWeatherData(application, zip); 
		application.interval = 8000;
		application.start();
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
		request.callback = function(message, value) {
		  if (5 == message) {
		  	value = JSON.parse(value);
			let icon = value.weather[0].icon.substring(0,2);
			let toDraw = 0;
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
			application.behavior.nextScreenData = {
			  city: value.name,
			  temp: value.main.temp + " F",
			  condition: titleCase(value.weather[0].description),
			  icon: toDraw
			};
			application.first.delegate("onTransitionOut");
		  }
		}
	  }
	  onAddNextScreen(application) {
	  	application.empty();
	  	application.purge();
	  	application.add(new MainCol(this.nextScreenData));
	  }
	},
});

function titleCase(str) {
	return str.split(' ').map(i => i[0].toUpperCase() + i.substring(1)).join(' ')
}
