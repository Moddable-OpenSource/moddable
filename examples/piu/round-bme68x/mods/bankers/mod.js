/*
 * Copyright (c) 2023-2024  Moddable Tech, Inc.
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
https://arnejacobsenwatches.com/aj_eur_en/wall/weather-station.html
*/

import Sensor from "embedded:sensor/Barometer-Humidity-Temperature/BME68x";
import {} from "piu/MC";
import Timeline from "piu/Timeline";

const BLACK = "black";
const LITE = "#D0D0D0";
const GRAY = "#808080";
const BLUE = "#192eab";
const WHITE = "#F8F8F8";
const RED = "#c83530"

const labelStyle = Object.freeze({ font:"semibold 10px Open Sans", color:GRAY, leading:-75 }, true);
const titleStyle = Object.freeze({ font:"light 16px Open Sans", color:GRAY }, true);
const valueStyle = Object.freeze({ font:"semibold 24px Open Sans", color:RED }, true);

class FromTimelineTransition extends Transition {
	constructor(Timeline) {
		super(0);
		this.Timeline = Timeline;
	}
	onBegin(container, former, $) {
		this.timeline = new this.Timeline(former, -1);
		this.duration = this.timeline.duration;
	}
	onEnd(container, former, $) {
		$.former = $.angle;
		this.timeline = null;
		container.remove(former);
		delete $.HAND;
		delete $.VALUE;
	}
	onStep(fraction) {
		this.timeline.fraction = 1 - fraction;
	}
};
Object.freeze(FromTimelineTransition);

class ToTimelineTransition extends Transition {
	constructor(Timeline) {
		super(0);
		this.Timeline = Timeline;
	}
	onBegin(container, Template, $) {
		container.purge();
		let former = new Template($);
		container.add(former);
		container.delegate("onTimeChanged");
		$.current = $.angle;
		this.timeline = new this.Timeline(former, 1);
		this.duration = this.timeline.duration;
	}
	onEnd(container) {
		this.timeline = null;
	}
	onStep(fraction) {
		this.timeline.fraction = fraction;
	}
}
Object.freeze(ToTimelineTransition);

class BankersBehavior extends Behavior {
	onCreate(application, $) {
		this.$ = $;
		this.which = 1;
		try {
			this.sensor = new Sensor({ sensor: device.I2C.default });
		}
		catch(e) {
			this.sensor = {
				sample() {
 					return {
					   thermometer: {
						  temperature: 33.68404006958008
					   },
					   hygrometer: {
						  humidity: 0.268742733001709
					   },
					   barometer: {
						  pressure: 100989.90625
					   }
				 	}
				}
			}
		}
		this.button = new device.peripheral.button.Flash({
			onPush: () => {
				if (this.button.pressed) {
					if (application.transitioning)
						return;
					let which = this.which + 1;
					if (which == 3)
						which = 0;
					this.which = which;
					let Template;
					if (this.which == 0)
						Template = BarometerContainer; 
					else if (this.which == 1)
						Template = HygrometerContainer; 
					else
						Template = ThermometerContainer; 
					application.run(new FromTimelineTransition(BankersTimeline), application.first, $);
					application.run(new ToTimelineTransition(BankersTimeline), Template, $);
				}
			}
		});
	}
	onDisplaying(application) {
		this.onTimeChanged(application);
		application.interval = 250;
		application.start();
	}
	onTimeChanged(application) {
		const sample = this.sensor.sample();
		if (sample)
			application.first.delegate("onSensorChanged", sample);
	}
};

class BarometerBehavior extends Behavior {
	onCreate(application, $) {
		this.$ = $;
		this.value = undefined;
	}
	onSensorChanged(container, sample) {
		const value = Math.max(950, Math.min(Math.round(sample.barometer.pressure / 100), 1070));
		if (this.value != value) {
			this.value = value;
			const fraction = (value - 950) / (1070 - 950);
			const a = (360 - (180 / 7) - (12 * (180 / 7) * fraction)) * Math.PI / 180;
			const shape = this.$.HAND;
			shape.behavior.onChanged(shape, a);
			const label = this.$.VALUE;
			label.string = value;
		}
	}
};

class HygrometerBehavior extends Behavior {
	onCreate(application, $) {
		this.$ = $;
		this.value = undefined;
	}
	onSensorChanged(container, sample) {
		const value = Math.round(sample.hygrometer.humidity * 100);
		if (this.value != value) {
			this.value = value;
			const fraction = value / 100;
			const a = (360 - 45 - (225 * fraction)) * Math.PI / 180;
			const shape = this.$.HAND;
			shape.behavior.onChanged(shape, a);
			const label = this.$.VALUE;
			label.string = value;
		}
	}
};

class ThermometerBehavior extends Behavior {
	onCreate(application, $) {
		this.$ = $;
		this.value = undefined;
	}
	onSensorChanged(container, sample) {
		const value = Math.max(-10, Math.min(Math.round(sample.thermometer.temperature), 50));
		if (this.value != value) {
			this.value = value;
			const fraction = (value + 10) / (50 + 10);
			const a = (360 - 45 - (270 * fraction)) * Math.PI / 180;
			const shape = this.$.HAND;
			shape.behavior.onChanged(shape, a);
			const label = this.$.VALUE;
			label.string = value;
		}
	}
};

class DialBehavior extends Behavior {
	onCreate(shape, $) {
		const fillPath = new Outline.CanvasPath();
		const strokePath = new Outline.CanvasPath();
		let a = $.a;
		for (let i = 0; i < $.i; i++) {
			const ar = a * Math.PI / 180;
			const cosa = Math.cos(ar);
			const sina = Math.sin(ar);
			let x = -2, y = 118, d = 4;
			for (let j = 0; j < $.j; j++) {
				const cosx = x * cosa;
				const cosxd = (x + d) * cosa;
				const sinx = x * sina;
				const sinxd = (x + d) * sina;
				const cosy = y * cosa;
				const cosyd = (y - d) * cosa;
				const siny = y * sina;
				const sinyd = (y - d) * sina;
			
				fillPath.moveTo(cosx + siny, cosy - sinx);
				fillPath.lineTo(cosxd + siny, cosy - sinxd);
				fillPath.lineTo(cosxd + sinyd, cosyd - sinxd);
				fillPath.lineTo(cosx + sinyd, cosyd - sinx);
				fillPath.lineTo(cosx + siny, cosy - sinx);
				
				if (j == i) {
					strokePath.moveTo(cosx + siny, cosy - sinx);
					strokePath.lineTo(cosxd + siny, cosy - sinxd);
					strokePath.lineTo(cosxd + sinyd, cosyd - sinxd);
					strokePath.lineTo(cosx + sinyd, cosyd - sinx);
					strokePath.lineTo(cosx + siny, cosy - sinx);
				}
				
				y -= 6;
			}
			a += $.da;
		}
		shape.fillOutline = Outline.fill(fillPath).translate(120, 120);
		shape.strokeOutline = Outline.fill(strokePath).translate(120, 120);
	}
};

class BackCenterBehavior extends Behavior {
	onCreate(shape) {
        this.outlines = new Array(11).fill();
        for (let i = 0; i < 11; i++) {
        	let path = new Outline.CanvasPath();
       	 	path.arc(0, 0, 20 + (10 * i), 0, 2 * Math.PI);
			this.outlines[i] = Outline.fill(path).translate(120, 120);	
        }
        shape.duration = 300;
    }
    onDisplaying(shape) {
    	this.onTimeChanged(shape);
    }
    onTimeChanged(shape) {
    	const i = Math.min(Math.floor(11 * shape.fraction), 10);
    	shape.fillOutline = this.outlines[i];	
    }
};

class HandBehavior extends Behavior {
	onChanged(shape, a) {
		const cx = shape.width >> 1;
		const cy = shape.height >> 1;
		shape.fillOutline = this.outline.clone().rotate(a).translate(cx, cy);
        this.$.angle = a;
	}
	onCreate(shape, $) {
        const path = new Outline.CanvasPath();
        path.moveTo(-2, -40);
        path.lineTo(2, -40);
        path.lineTo(2, 116);
        path.lineTo(-2, 116);
        path.lineTo(-2, -40);
        path.closePath();
        this.outline = Outline.fill(path);
        this.$ = $;
        shape.duration = 300;
    }
    onTimeChanged(shape) {
    	const { former, current } = this.$;
    	this.onChanged(shape, former + ((current - former) * (1 - shape.fraction)));
    }
};

class ForeCenterBehavior extends Behavior {
	onCreate(shape) {
        this.outlines = new Array(10).fill();
        let delta = (120 - 16) / 10;
        for (let i = 0; i < 10; i++) {
        	let path = new Outline.CanvasPath();
       	 	path.arc(0, 0, 16 + (delta * i), 0, 2 * Math.PI);
			this.outlines[i] = Outline.fill(path).translate(120, 120);	
        }
        shape.duration = 300;
    }
    onDisplaying(shape) {
    	this.onTimeChanged(shape);
    }
    onTimeChanged(shape) {
    	const i = Math.min(Math.floor(10 * shape.fraction), 9);
    	shape.fillOutline = this.outlines[i];	
    }
};



const BarometerContainer = Container.template($ => ({
	left:0, top:0, width:240, height:240, Behavior:BarometerBehavior,
	contents: [
		Shape({ a:2*180/7, da:180/7, i:11, j:11}, { left:0, top:0, width:240, height:240, Behavior:DialBehavior, skin:{ fill:LITE, stroke:BLACK } } ),
		Text($, { left:78, width:20, top:140, style:labelStyle, string:"09\n60" } ),
		Text($, { left:68, width:20, top:100, style:labelStyle, string:"09\n80" } ),
		Text($, { left:90, width:20, top:76, style:labelStyle, string:"10\n00" } ),
		Text($, { right:90, width:20, top:76, style:labelStyle, string:"10\n20" } ),
		Text($, { right:68, width:20, top:100, style:labelStyle, string:"10\n40" } ),
		Text($, { right:78, width:20, top:140, style:labelStyle, string:"10\n60" } ),
		Shape($, { width:240, height:240, Behavior:BackCenterBehavior, skin:{ fill:WHITE } } ),
		Shape($, { anchor:"HAND", width:240, height:240, Behavior:HandBehavior, skin:{ fill:BLACK } } ),
		Shape($, { width:240, height:240, Behavior:ForeCenterBehavior, skin:{ fill:RED } } ),
		Label($, { top:165, style:titleStyle, string:"BARO" } ),
		Label($, { top:180, anchor:"VALUE", style:valueStyle } ),
		Label($, { top:205, style:titleStyle, string:"hPa" } ),
	]
}));

const HygrometerContainer = Container.template($ => ({
	left:0, top:0, width:240, height:240, Behavior:HygrometerBehavior,
	contents: [
		Shape({ a:90, da:22.5, i:9, j:10}, { left:0, top:0, width:240, height:240, Behavior:DialBehavior, skin:{ fill:LITE, stroke:BLACK } } ),
		Label($, { left:65, top:112, style:labelStyle, string:"20" } ),
		Label($, { left:80, top:78, style:labelStyle, string:"40" } ),
		Label($, { top:65, style:labelStyle, string:"60" } ),
		Label($, { right:80, top:78, style:labelStyle, string:"80" } ),
		Label($, { right:65, top:112, style:labelStyle, string:"100" } ),
		Shape($, { width:240, height:240, Behavior:BackCenterBehavior, skin:{ fill:WHITE } } ),
		Shape($, { anchor:"HAND", width:240, height:240, Behavior:HandBehavior, skin:{ fill:BLACK } } ),
		Shape($, { width:240, height:240, Behavior:ForeCenterBehavior, skin:{ fill:RED } } ),
		Label($, { top:165, style:titleStyle, string:"HYGRO" } ),
		Label($, { top:180, anchor:"VALUE", style:valueStyle } ),
		Label($, { top:205, style:titleStyle, string:"%rel." } ),
	]
}));

const ThermometerContainer = Container.template($ => ({
	left:0, top:0, width:240, height:240, Behavior:ThermometerBehavior,
	contents: [
		Shape({ a:90-22.5, da:22.5, i:11, j:11}, { left:0, top:0, width:240, height:240, Behavior:DialBehavior, skin:{ fill:LITE, stroke:BLACK } } ),
		Label($, { left:72, top:112, style:labelStyle, string:"0" } ),
		Label($, { left:85, top:82, style:labelStyle, string:"10" } ),
		Label($, { top:70, style:labelStyle, string:"20" } ),
		Label($, { right:85, top:82, style:labelStyle, string:"30" } ),
		Label($, { right:72, top:112, style:labelStyle, string:"40" } ),
		Shape($, { width:240, height:240, Behavior:BackCenterBehavior, skin:{ fill:WHITE } } ),
		Shape($, { anchor:"HAND", width:240, height:240, Behavior:HandBehavior, skin:{ fill:BLACK } } ),
		Shape($, { width:240, height:240, Behavior:ForeCenterBehavior, skin:{ fill:RED } } ),
		Label($, { top:165, style:titleStyle, string:"THERMO" } ),
		Label($, { top:180, anchor:"VALUE", style:valueStyle } ),
		Label($, { top:205, style:titleStyle, string:"°C" } ),
	]
}));

class BankersTimeline extends Timeline {
	constructor(screen, direction) {
		super();
		let unit = screen.last;
		let value = unit.previous;
		let title = value.previous;
		let fore = title.previous;
		let hand = fore.previous;
		let back = hand.previous;
		if (direction > 0)
			this.from(hand, { time:hand.duration }, hand.duration, Math.quadEaseOut, 0);
		this.from(back, { time:back.duration }, back.duration, Math.quadEaseOut, 0);
		this.from(title, { y:screen.y + screen.height }, 100, Math.quadEaseOut, 0);
		this.from(value, { y:screen.y + screen.height }, 100, Math.quadEaseOut, 0);
		this.from(unit, { y:screen.y + screen.height }, 100, Math.quadEaseOut, 0);
	}
}

let BankersApplication = Application.template($ => ({
	skin:{ fill:WHITE }, 
	Behavior:BankersBehavior,
	contents: [
		HygrometerContainer($, {})
	]
}));

export default function () {
	return new BankersApplication({}, { pixels: screen.width * 16, displayListLength:4096, touchCount:0 });
}

