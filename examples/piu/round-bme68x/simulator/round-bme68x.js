/*
 * Copyright (c) 2023-2024 Moddable Tech, Inc.
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
	Screen,
} from "piu/Screen";

import {
	ControlsColumn,
	ButtonsRow,
	InfoRow,
	PopupRow,
	SliderRow,
	StatusRow,
	SwitchRow,
	TimerRow,
} from "ControlsPane";

import {
	DeviceBehavior,
	DeviceContainer,
	DeviceScreen,
	DeviceWorker,
} from "DevicePane";

const base = import.meta.uri;
const ledSkin = { texture:{ base, path:"round-bme68x/screen.png" }, x:0, y:0, width:160, height:160 };

class MockupBehavior extends DeviceBehavior {
	onAbort(container, status) {
		if (this.reloadOnAbort) {
			application.defer("doReloadFile");
			return true;
		}
	}
	onConfigure(container) {
		this.onSensorChanged(container);
	}
	onCreate(container, device) {
		super.onCreate(container, device);
		container.duration = 0;
		container.interval = 250;
		this.humidity = 50 / 100;
		this.pressure = 1000 * 100;
		this.temperature = 20;
	}
	onDefaultButtonDown(container) {
		this.postJSON(container, { button:1 });
	}
	onDefaultButtonUp(container) {
		this.postJSON(container, { button:0 });
	}
	onFinished(container) {
	}
	onHumidityChanged(container, data) {
		this.humidity = data.value / 100;
		this.onSensorChanged(container);
	}
	onJSON(container, json) {
		if ("xsbug" in json) {
			if (json.xsbug == "abort")
				application.defer("doReloadFile");
		}
	}
	onReloadOnAbortChanged(container, data) {
		this.reloadOnAbort = data.value;
	}
	onPressureChanged(container, data) {
		this.pressure = data.value * 100;
		this.onSensorChanged(container);
	}
	onTemperatureChanged(container, data) {
		this.temperature = data.value;
		this.onSensorChanged(container);
	}
	onSensorChanged(container) {
		const { temperature, humidity, pressure } = this;
		this.postJSON(container, { 
			bme68x: { 
				thermometer: { temperature },
				hygrometer: { humidity },
				barometer: { pressure }
			}
		});
	}
	onTimeChanged(container) {
	}
}

class LEDBehavior extends Behavior {
	onLEDChanged(content, state) {
		content.visible = state;
	}
}

export default {
	applicationName: "round-bme68x",
	title:"Round BME68X",
	Workers: {
	},
	ControlsTemplate:ControlsColumn.template($ => ({
		contents:[
			SliderRow({ 
				name: "TEMPERATURE",
				track: "onTemperatureChanged",
				label: "Temperature",
				min: -40,
				max: 85,
				step: 1,
				unit: "°C",
				value: 20
			}),
			SliderRow({ 
				name: "HUMIDITY",
				track: "onHumidityChanged",
				label: "Humidity",
				min: 0,
				max: 100,
				step: 1,
				unit: "% RH",
				value: 50
			}),
			SliderRow({ 
				name: "PRESSURE",
				track: "onPressureChanged",
				label: "Pressure",
				min: 300,
				max: 1100,
				step: 1,
				unit: " hPa",
				value: 1000
			}),
			ButtonsRow({ 
				label: "Button",
				buttons: [
					{ eventDown:"onDefaultButtonDown", eventUp:"onDefaultButtonUp", label:"Default" },
				],
			}),
			SwitchRow({
				event: "onReloadOnAbortChanged",
				label: "Reload On Abort",
				on: "Yes",
				off: "No",
				value: false
			}),
		]
	})),
	DeviceTemplates: {
		0: DeviceContainer.template($ => ({ 
			Behavior: MockupBehavior, 
			contents:[
				Content($, { skin: { texture:{ base, path:"assets/0.png" }, x:0, y:0, width:290, height:310 } }),
				DeviceScreen($, { left:25, width:240, top:25, height:240, rotation:0, circular:true }),
			],
		})),
		90: DeviceContainer.template($ => ({ 
			Behavior: MockupBehavior, 
			contents:[
				Content($, { skin: { texture:{ base, path:"assets/90.png" }, x:0, y:0, width:310, height:290 } }),
				DeviceScreen($, { left:25, width:240, top:25, height:240, rotation:90, circular:true }),
			],
		})),
		180: DeviceContainer.template($ => ({ 
			Behavior: MockupBehavior, 
			contents:[
				Content($, { skin: { texture:{ base, path:"assets/180.png" }, x:0, y:0, width:290, height:310 } }),
				DeviceScreen($, { left:25, width:240, top:45, height:240, rotation:180, circular:true }),
			],
		})),
		270: DeviceContainer.template($ => ({ 
			Behavior: MockupBehavior,
			contents:[
				Content($, { skin: { texture:{ base, path:"assets/270.png" }, x:0, y:0, width:310, height:290 } }),
				DeviceScreen($, { left:45, width:240, top:25, height:240, rotation:270, circular:true }),
			],
		})),
	}
};
