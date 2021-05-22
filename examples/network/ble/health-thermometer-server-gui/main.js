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
import Timeline from "piu/Timeline";
import HealthThermometerService from "htmservice";

const WHITE = "white";

const btIconSkin = new Skin({ texture:new Texture("sm-bt-logo.png"), color:WHITE, x:0, y:0, width:20, height:35 });
const arcSkin = new Skin({ texture:new Texture("color-arc.png"), x:0, y:0, width:240, height:58 });
const ticksSkin = new Skin({ texture:new Texture("ticks-arc.png"), color:WHITE, x:0, y:0, width:240, height:48 });
const dotSkin = new Skin({ texture:new Texture("dot.png"), color:WHITE, x:0, y:0, width:10, height:10 });
const labelStyle = new Style({ font:"18px Open Sans", color:WHITE, horizontal:"left", vertical:"top" });
const temperatureStyle = new Style({ font:"72px Open Sans", color:WHITE, horizontal:"center", vertical:"top" });

const DotContent = Content.template($ => ({
	skin: dotSkin
}))

class HTMApplicationBehavior extends Behavior {
	onCreate(application, anchors) {
		this.anchors = anchors;
		this.htm = new HealthThermometerService;
		this.minTemp = 96;
		this.maxTemp = 104;
		this.minDegrees = 77;
		this.maxDegrees = 103;
		this.radius = 250;
		this.f = Math.PI / 180;
	}
	onConnected(application) {
		let anchors = this.anchors;
		this.reverse = false;
		anchors.TEMPERATURE.string = "-°";
		anchors.DOT.visible = true;
		this.timeline = (new Timeline)
			.to(anchors.ICON, { x:16, y:12 }, 300, Math.quadEaseOut, 0)
			.to(anchors.TITLE, { x:45, y:17 }, 300, Math.quadEaseOut, -200)
			.to(anchors.ARC, { y:72 }, 250, Math.quadEaseOut, 0)
			.to(anchors.DOT, { y:72 }, 250, Math.quadEaseOut, -100)
			.to(anchors.TICKS, { y:150 }, 250, Math.quadEaseOut, -100)
			.to(anchors.TEMPERATURE, { y:188 }, 250, Math.quadEaseOut, -100)
		application.time = 0;
		application.duration = this.timeline.duration + 500;
		application.start();	
	}
	onDisconnected(application) {
		if ("timeline" in this) {
			this.reverse = true;
			application.time = 0;
			application.start();
		}
	}
	onTimeChanged(application) {
		let time = application.time;
		if (this.reverse) {
			time = application.duration - time;
			if (time < 1000)
				this.anchors.DOT.x = -10;
		}
		this.timeline.seekTo(time);
	}
	onValue(application, value) {
		if (value == this.lastValue) return;
		this.lastValue = value;
		let temperature = value;
		let radians = this.tempToRadians(temperature);
		
		// map left coordinate between left and right temperature labels
		let x = Math.cos(radians) * this.radius;
		let left = (x+56)/(112) * (210-22) + 22;
		
		// map top coordinate from (0, 0) based circle to screen
		// y values for the displayed temperature range are between [243.6, 250]
		// dot vertical positions are between [130, 150]
		let y = Math.sin(radians) * this.radius;
		let top = (y - 243.6)/(250-243.6) * (130-150) + 150;
		
		let dot = this.anchors.DOT;
		dot.moveBy(left - dot.x, top - dot.y);
		this.anchors.TEMPERATURE.string = value.toFixed(1) + "°";
	}
	tempToRadians(temperature) {
		// angle (a) in degrees for line that passes through (96, 103) and (104, 77):
		// a = -13/4 * t + 415
		return (-13./4 * temperature + 415) * this.f;
	}
};

let HTMApplication = Application.template($ => ({
	skin: new Skin({ fill:"black" }),
	Behavior: HTMApplicationBehavior,
	style:labelStyle,
	contents: [
		Content($, { anchor:"ICON", left:110, top:136, skin:btIconSkin }),
		Label($, { anchor:"TITLE", left:32, top:175, width:180, string:"Health Thermometer" }),
		Container($, {
			anchor:"ARC", left:0, right:0, top:screen.height, height:86,
			contents: [
				Content($, { bottom:0, skin:arcSkin }),
				Label($, { left:8, top:16, string:"96°" }),
				Label($, { left:102, top:0, string:"100°" }),
				Label($, { left:196, top:16, string:"104°" }),
			]
		}),
		DotContent($, { anchor:"DOT", left:-10, top:screen.height }),
		Content($, { anchor:"TICKS", left:0, top:screen.height, skin:ticksSkin }),
		Label($, { anchor:"TEMPERATURE", top:screen.height, style:temperatureStyle }),
	]
}));

export default function () {
	new HTMApplication({}, { touchCount:0 });
}
