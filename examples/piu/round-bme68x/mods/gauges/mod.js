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

import Sensor from "embedded:sensor/Barometer-Humidity-Temperature/BME68x";
import {} from "piu/MC";

const BLACK = "black";
const LITE = "#D0D0D0";
const GRAY = "#808080";
const BLUE = "#192eab";
const WHITE = "#F8F8F8";
const RED = "#c83530"
const CYAN = "#00FFFF"
const MAGENTA = "#FF00FF"
const YELLOW = "#FFFF00"

const titleStyle = Object.freeze({ font:"24px FiraSansCondensed", color:LITE }, true);
const unitStyle = Object.freeze({ font:"24px FiraSansCondensed", color:[CYAN,MAGENTA,YELLOW] }, true);
const valueStyle = Object.freeze({ font:"bold 32px FiraSansCondensed", color:[CYAN,MAGENTA,YELLOW] }, true);

const sqrt3 = Math.sqrt(3);
const cx = 120;
const cy = 160;
const radius = cy / (1 + (2 * sqrt3 / 3));
const diameter = Math.round(2 * radius);

class GaugesBehavior extends Behavior {
	onCreate(application, data) {
		this.data = data;
		this.humidity = undefined;
		this.pressure = undefined;
		this.temperature = undefined;
		this.rotating = 0;
		this.centers = [{x:0,y:0},{x:0,y:0},{x:0,y:0}];
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
					if (this.rotating)
						return;
					application.stop();
					this.rotating = true;
					for (let i = 0; i < 3; i++) {
						let center = this.centers[i];
						let container = application.content(i);
						center.x = container.x - application.x - cx + radius;
						center.y = container.y - application.y - cy + radius;
					}
					application.duration = 1000;
					application.interval = 1;
					application.time = 0;
					application.start();
				}
			}
		});
	}
	onDisplaying(application) {
		this.onTimeChanged(application);
		application.interval = 250;
		application.start();
	}
	onFinished(application) {
		this.rotating = false;
		application.duration = 0;
		application.interval = 250;
		application.start();
	}
	onTimeChanged(application) {
		if (this.rotating) {
			let fraction = Math.quadEaseOut(application.fraction);
			let angle = fraction * 2 * Math.PI / 3;
			const cosa = Math.cos(angle);
			const sina = Math.sin(angle);
			for (let i = 0; i < 3; i++) {
				let { x, y } = this.centers[i];
				let container = application.content(i);
				container.x = Math.round(application.x + cx - radius + (cosa * x) + (sina * y));
				container.y = Math.round(application.y + cy - radius + (cosa * y) - (sina * x));
			}
			return;
		}
		const sample = this.sensor.sample();
		if (sample) {
			const data = this.data;
			let value, fraction;
			value = Math.round(sample.hygrometer.humidity * 100);
			if (this.humidity != value) {
				this.humidity = value;
				fraction = value / 100;
				application.distribute("onSensorChanged", "humidity", value, fraction);
			}
			value = Math.max(950, Math.min(Math.round(sample.barometer.pressure / 100), 1070));
			if (this.pressure != value) {
				this.pressure = value;
				fraction = (value - 950) / (1070 - 950);
				application.distribute("onSensorChanged", "pressure", value, fraction);
			}			
			value = Math.max(-10, Math.min(Math.round(sample.thermometer.temperature), 50));
			if (this.temperature != value) {
				this.temperature = value;
				fraction = (value + 10) / (50 + 10);
				application.distribute("onSensorChanged", "temperature", value, fraction);
			}
		}
	}
};

class GaugeBehavior extends Behavior {
	onCreate(shape, data) {
        this.data = data;
    }
	onSensorChanged(container, which, value, fraction) {
		if (this.data.which != which)
			return;
		const shape = container.first;
		const label = shape.next;
		const { width, height } = shape;
		const x = radius;
		const y = radius;
		const r = radius;
		const twelfth = Math.PI / 6;
		const weight = this.data.weight;
		
		if (shape.fillOutline == null) {
			const path = new Outline.CanvasPath;
			let angle = 5 * twelfth;
			const limit = 13 * twelfth;
			const delta = Math.PI / 60;
			while (angle < limit) {
				path.arc(x, y, r - (weight >> 1), angle, angle + delta);
				angle += delta;
				angle += delta;
			}
			shape.fillOutline = Outline.stroke(path, weight, Outline.LINECAP_BUTT, Outline.LINEJOIN_MITER);
		}
		const path = new Outline.CanvasPath;
		let angle = 5 * twelfth;
		const limit = 5 * twelfth + (8 * twelfth * fraction);
		const delta = Math.PI / 60;
		while (angle < limit) {
			path.arc(x, y, r - (weight >> 1), angle, angle + delta);
			angle += delta;
			angle += delta;
		}
// 		
// 		
// 		path.arc(x, y, r - (weight >> 1), 5 * twelfth, 5 * twelfth + (8 * twelfth * fraction) );
		shape.strokeOutline = Outline.stroke(path, weight, Outline.LINECAP_BUTT, Outline.LINEJOIN_MITER);
		
		label.string = value;
	}
};

let GaugesApplication = Application.template($ => ({
	skin:{ fill:BLACK }, 
	Behavior:GaugesBehavior,
	contents: [
		Container({ which:"pressure", weight:12 }, {
			left:Math.round(cx - radius), top:0, width:diameter, height:diameter, Behavior:GaugeBehavior,
			contents: [
				Shape($, { left:0, right:0, top:0, bottom:0, skin:{ fill:LITE, stroke:[CYAN,MAGENTA,YELLOW] } } ),
				Label($, { top:30, style:valueStyle } ),
				Label($, { top:65, style:unitStyle, string:"hPa" } ),
				Label($, { top:95, style:titleStyle, string:"BARO" } ),
			]
		}),
		Container({ which:"temperature", weight:12 }, {
			right:cx, top:Math.round(sqrt3*radius), width:diameter, height:diameter, Behavior:GaugeBehavior,
			contents: [
				Shape($, { left:0, right:0, top:0, bottom:0, state:1, skin:{ fill:LITE, stroke:[CYAN,MAGENTA,YELLOW] } } ),
				Label($, { top:30, style:valueStyle, state:1 } ),
				Label($, { top:65, style:unitStyle, state:1, string:"°C" } ),
				Label($, { top:95, style:titleStyle, string:"THERMO" } ),
			]
		}),
		Container({ which:"humidity", weight:12 }, {
			left:cx, top:Math.round(sqrt3*radius), width:diameter, height:diameter, Behavior:GaugeBehavior,
			contents: [
				Shape($, { left:0, right:0, top:0, bottom:0, state:2, skin:{ fill:LITE, stroke:[CYAN,MAGENTA,YELLOW] } } ),
				Label($, { top:30, style:valueStyle, state:2 } ),
				Label($, { top:65, style:unitStyle, state:2, string:"%" } ),
				Label($, { top:95, style:titleStyle, string:"HYGRO" } ),
			]
		})
	]
}));

export default function () {
	return new GaugesApplication({}, { pixels: screen.width * 16, displayListLength:4096, touchCount:0 });
}

