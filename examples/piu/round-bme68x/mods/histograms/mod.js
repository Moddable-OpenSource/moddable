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
const RED = "#ab1919"
const CYAN = "#00FFFF"
const MAGENTA = "#FF00FF"
const YELLOW = "#FFFF00"
const DARK_CYAN = "#008080"
const DARK_MAGENTA = "#800080"
const DARK_YELLOW = "#808000"
const GREEN = "#19ab19"
const MASK = "#000000A0"

const valueStyle = Object.freeze({ font:"bold 20px FiraSansCondensed", color:WHITE }, true);

class HistogramsBehavior extends Behavior {
	onCreate(application, data) {
		this.data = data;
		data.humidity = undefined;
		data.pressure = undefined;
		data.temperature = undefined;
		data.samplesCount = 60;
		data.samplesIndex = -1;
		data.samplesLoop = 0;
		data.samples = {
			humidity: new Uint8Array(data.samplesCount),
			pressure: new Uint8Array(data.samplesCount),
			temperature: new Uint8Array(data.samplesCount),
		};
		
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
					application.stop();
					let interval = application.interval * 2;
					if (interval > 2000)
						interval = 250;
					application.interval = interval;
					application.start();
					application.distribute("onRateChanged", interval);
				}
			}
		});
	}
	onDisplaying(application) {
		this.onTimeChanged(application);
		application.interval = 500;
		application.start();
	}
	onTimeChanged(application) {
		const sample = this.sensor.sample();
		if (sample) {
			const data = this.data;
			var samplesIndex = data.samplesIndex + 1;
			if (samplesIndex >= data.samplesCount) {
				data.samplesLoop = true;
				samplesIndex = 0;
			}
			data.samplesIndex = samplesIndex;
			
			let value, fraction;
			
			value = Math.round(sample.hygrometer.humidity * 100);
			fraction = value / 100;
			data.humidity = value;
			data.samples.humidity[samplesIndex] = Math.round(fraction * 255);

			value = Math.round(sample.barometer.pressure / 100);
			fraction = (value - 300) / (1100 - 300);
			data.pressure = value;
			data.samples.pressure[samplesIndex] = Math.round(fraction * 255);

			value = Math.round(sample.thermometer.temperature);
			fraction = (value + 40) / (85 + 40);
			data.temperature = value;
			data.samples.temperature[samplesIndex] = Math.round(fraction * 255);
			
			application.distribute("onSensorChanged", data);
		}
	}
};

class HistogramBehavior extends Behavior {
	onCreate(container, data) {
        this.data = data;
    }
	onSensorChanged(container, data) {
		const { which, weight } = this.data;
		const shape = container.first;
		const label = shape.next;
		const { width, height } = shape;
		const x = width >> 1;
		const y = height >> 1;
		const radius = Math.min(x, y);
		
		if (shape.fillOutline == null) {
			const path = new Outline.CanvasPath;
			path.arc(x, y, radius, 0, 2 * Math.PI);
			path.arc(x, y, radius - weight + 1, 0, 2 * Math.PI);
			shape.fillOutline = Outline.fill(path, Outline.EVEN_ODD_RULE);
		}
		
		const path = new Outline.CanvasPath;
		
		const samples = data.samples[which];
		let { samplesCount, samplesIndex, samplesLoop } = data;
		const samplesLimit = samplesIndex;
		let angle = Math.PI / 2;
		const delta = 2 * Math.PI / samplesCount;
		const factor = (weight - 2) / 255;
		
		for (let index = 0; index < samplesCount; index++) {
			let offset = samplesIndex - index;
			if (offset < 0) {
				offset = samplesCount + offset;
			}
			const level = radius - weight + 2 + samples[offset] * factor;
			const x = radius + Math.cos(angle) * level;
			const y = radius - Math.sin(angle) * level;
			if (index == 0)
				path.moveTo(x, y);
			else	
				path.lineTo(x, y);
			angle += delta;
		}
		path.closePath();

		path.arc(x, y, radius - weight, 0, 2 * Math.PI, true);
		shape.strokeOutline = Outline.fill(path, Outline.EVEN_ODD_RULE);
	}
};

class ValuesBehavior extends Behavior {
	onCreate(container, data) {
		const shape = container.first
		const path = new Outline.CanvasPath;
		path.moveTo(120, 120);
		path.arc(120, 120, 120, 1 * Math.PI / 4, 3 * Math.PI / 4);
		path.closePath();
		shape.fillOutline = Outline.fill(path);
    }
	onSensorChanged(container, data) {
		const shape = container.first;
		let label = shape.next;
		label.string = data.pressure + " hPa";
 		label = label.next;
		label.string = data.temperature + "°C";
 		label = label.next;
		label.string = data.humidity + "%";
   }
};

class RateBehavior extends Behavior {
	onCreate(shape, data) {
		const path = new Outline.CanvasPath;
		path.arc(15, 15, 15, 0, 2 * Math.PI, true);
		shape.fillOutline = Outline.fill(path);
    }
    onDisplaying(shape) {
    	shape.duration = 500;
    	shape.start();
    }
    onFinished(shape) {
    	shape.time = 0;
    	shape.start();
    }
    onRateChanged(shape, rate) {
    	shape.stop();
    	shape.duration = rate;
    	shape.time = 0;
    	shape.start();
    }
    onTimeChanged(shape) {
    	shape.state = shape.fraction;
    }
};

let HistogramsApplication = Application.template($ => ({
	skin:{ fill:BLACK }, 
	Behavior:HistogramsBehavior,
	contents: [
		Container({ which:"pressure", weight:34, unit:" hPa" }, {
			left:0, top:0, width:240, height:240, Behavior:HistogramBehavior,
			contents: [
				Shape($, { left:0, right:0, top:0, bottom:0, skin:{ fill:DARK_CYAN, stroke:CYAN } } ),
			]
		}),
		Container({ which:"temperature", weight:34, unit:"°C" }, {
			left:34, top:34, width:172, height:172, Behavior:HistogramBehavior,
			contents: [
				Shape($, { left:0, right:0, top:0, bottom:0, state:1, skin:{ fill:DARK_MAGENTA, stroke:MAGENTA } } ),
			]
		}),
		Container({ which:"humidity", weight:34, unit:"%" }, {
			left:68, top:68, width:104, height:104, Behavior:HistogramBehavior,
			contents: [
				Shape($, { left:0, right:0, top:0, bottom:0, state:2, skin:{ fill:DARK_YELLOW, stroke:YELLOW } } ),
			]
		}),
		Container($, {
			left:0, right:0, top:0, height:240, Behavior:ValuesBehavior,
			contents: [
				Shape($, { left:0, right:0, top:0, bottom:0, skin:{ fill:MASK } } ),
				Label($, { bottom:7, style:valueStyle } ),
				Label($, { bottom:41, style:valueStyle } ),
				Label($, { bottom:75, style:valueStyle } ),
			]
		}),		
		Content($, { left:119, width:2, top:0, height:120, skin:{ fill:MASK } }),
		Shape($, { width:30, top:105, height:30, skin:{ fill:[BLACK,LITE] }, Behavior:RateBehavior } ),
	]
}));

export default function () {
	return new HistogramsApplication({}, { pixels: screen.width * 16, displayListLength:4096, touchCount:0 });
}

