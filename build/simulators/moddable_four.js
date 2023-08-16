/*
 * Copyright (c) 2023  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Tools.
 * 
 *   The Moddable SDK Tools is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Tools is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 * 
 *   You should have received a copy of the GNU General Public License
 *   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
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
const ledSkin = { texture:{ base, path:"moddable_four/led.png" }, x:0, y:0, width:160, height:160 };

class MockupBehavior extends DeviceBehavior {
	onAbort(container, status) {
		if (this.reloadOnAbort) {
			application.defer("doReloadFile");
			return true;
		}
	}
	onAccelerometerChanged(container) {
		if (this.sleeping)
			return;
		const { x, y, z } = this;
		this.postJSON(container, { accelerometer: { x, y, z } });
	}
	onAccelerometerFlat(container) {
		this.x = 0;
		this.y = 0;
		this.z = 9.81;
		const xSlider = this.X;
		const ySlider = this.Y;
		const zSlider = this.Z;
		xSlider.behavior.data.value = 0;
		xSlider.behavior.onDataChanged(xSlider);
		xSlider.behavior.onValueChanging(xSlider);
		ySlider.behavior.data.value = 0;
		ySlider.behavior.onDataChanged(ySlider);
		ySlider.behavior.onValueChanging(ySlider);
		zSlider.behavior.data.value = 100;
		zSlider.behavior.onDataChanged(zSlider);
		zSlider.behavior.onValueChanging(zSlider);
		this.onAccelerometerChanged(container);
	}
	onAccelerometerShake(container) {
		if (this.sleeping)
			this.onWake(container, "accelerometer");
	}
	onAccelerometerXChanged(container, data) {
		this.x = data.value * 9.81 / 100;
		this.onAccelerometerChanged(container);
	}
	onAccelerometerYChanged(container, data) {
		this.y = data.value * 9.81 / 100;
		this.onAccelerometerChanged(container);
	}
	onAccelerometerZChanged(container, data) {
		this.z = data.value * 9.81 / 100;
		this.onAccelerometerChanged(container);
	}
	onConfigure(container) {
		if (this.sleeping) {
			this.postJSON(container, { retain: this.retain, wakenWith: this.wakenWith });
			
			this.retain = [];
			this.sleeping = false;
			this.wakenWith = "";
		}
	}
	onCreate(container, device) {
		super.onCreate(container, device);
		container.duration = 0;
		container.interval = 250;
		this.push = 0;
		this.turn = 0;
		this.x = 0;
		this.y = 0;
		this.z = 1;
		this.reloadOnAbort = false;
		
		this.retain = [];
		this.sleeping = false;
		this.wakenWith = "";
	}
	onFinished(container) {
	}
	onKeyDown(container, key) {
		const code = key.charCodeAt(0);
		if (code == 127)
			this.onBackButtonDown(container);
		else if (code == 13)
			this.onEnterButtonDown(container);
		else if (code == 63232)
			this.onTurnCWJogDialDown(container);
		else if (code == 63233)
			this.onTurnCCWJogDialDown(container);
	}
	onKeyUp(container, key) {
		const code = key.charCodeAt(0);
		if (code == 127)
			this.onBackButtonUp(container);
		else if (code == 13)
			this.onEnterButtonUp(container);
		else if (code == 63232)
			this.onTurnCWJogDialUp(container);
		else if (code == 63233)
			this.onTurnCCWJogDialUp(container);
	}
	onJSON(container, json) {
		if ("led" in json)
			container.distribute("onLEDChanged", json.led);
		else if ("sleep" in json) {
			this.sleeping = true;
			this.retain = json.retain;
			const control = this.SLEEP_RETAIN;
			control.string = "[" + json.retain + "]";
			this.wakeWith = json.wakeWith;
			if (json.sleep) {
				let timer = this.SLEEP_TIMER;
				timer.duration = json.sleep;
				timer.time = 0;
				timer.start();
			}
		}
		else if ("xsbug" in json) {
			if (json.xsbug == "abort")
				application.defer("doReloadFile");
		}
	}
	onPushCWJogDialDown(container) {
		if (this.sleeping)
			return;
		container.start();
		this.postJSON(container, { jogdial: { push:1, turn:4 } });
		this.push = 1;
		this.turn = 1;
	}
	onPushCWJogDialUp(container) {
		if (this.sleeping)
			return;
		container.stop();
	}
	onPushCCWJogDialDown(container) {
		if (this.sleeping)
			return;
		container.start();
		this.postJSON(container, { jogdial: { push:1, turn:-4 } });
		this.push = 1;
		this.turn = -1;
	}
	onPushCCWJogDialUp(container) {
		if (this.sleeping)
			return;
		container.stop();
	}
	onReloadOnAbortChanged(container, data) {
		this.reloadOnAbort = data.value;
	}
	onSleepTimerChanged(container) {
	}
	onSleepTimerFinished(container) {
		this.onWake(container, "timer");
	}
	onTimeChanged(container) {
		this.postJSON(container, { jogdial: { push:this.push, turn:this.turn }});
	}
	onTurnCWJogDialDown(container) {
		if (this.sleeping)
			return;
		container.start();
		this.postJSON(container, { jogdial: { push:0, turn:4 } });
		this.push = 0;
		this.turn = 1;
	}
	onTurnCWJogDialUp(container) {
		if (this.sleeping)
			return;
		container.stop();
	}
	onTurnCCWJogDialDown(container) {
		if (this.sleeping)
			return;
		container.start();
		this.postJSON(container, { jogdial: { push:0, turn:-4 } });
		this.push = 0;
		this.turn = -1;
	}
	onTurnCCWJogDialUp(container) {
		if (this.sleeping)
			return;
		container.stop();
	}
	onBackButtonDown(container) {
		if (this.sleeping)
			return;
		this.postJSON(container, { button:1 });
	}
	onBackButtonUp(container) {
		if (this.sleeping) {
			this.onWake(container, "button");
			return;
		}
		this.postJSON(container, { button:0 });
	}
	onEnterButtonDown(container) {
		if (this.sleeping)
			return;
		this.postJSON(container, { jogdial: { push:0 } });
	}
	onEnterButtonUp(container) {
		if (this.sleeping) {
			this.onWake(container, "jogdial");
			return;
		}
		this.postJSON(container, { jogdial: { push:1 } });
	}
	onWake(container, wakenWith) {
		let timer = this.SLEEP_TIMER;
		timer.stop();
		timer.duration = 0;
		timer.time = 0;
		this.wakenWith = wakenWith;
		application.defer("doReloadFile");
	}
}

class LEDBehavior extends Behavior {
	onLEDChanged(content, state) {
		content.visible = state;
	}
}

export default {
	applicationName: "moddable_four/debug/.*",
	sortingTitle:"0003",
	title:"Moddable Four",
	Workers: {
	},
	ControlsTemplate:ControlsColumn.template($ => ({
		contents:[
			ButtonsRow({ 
				label: "Buttons",
				buttons: [
					{ eventDown:"onBackButtonDown", eventUp:"onBackButtonUp", label:"Back" },
					{ eventDown:"onEnterButtonDown", eventUp:"onEnterButtonUp", label:"Enter" },
				],
			}),
			ButtonsRow({ 
				label: "Jog Dial",
				buttons: [
					{ eventDown:"onTurnCWJogDialDown", eventUp:"onTurnCWJogDialUp", label:"Turn CW" },
					{ eventDown:"onTurnCCWJogDialDown", eventUp:"onTurnCCWJogDialUp", label:"Turn CCW" },
				],
			}),
// 			ButtonsRow({ 
// 				label: "",
// 				buttons: [
// 					{ eventDown:"onPushCWJogDialDown", eventUp:"onPushCWJogDialUp", label:"Push CW" },
// 					{ eventDown:"onPushCCWJogDialDown", eventUp:"onPushCCWJogDialUp", label:"Push CCW" },
// 				],
// 			}),
			ButtonsRow({ 
				label: "Accelerometer",
				buttons: [
					{ event:"onAccelerometerFlat", label:"Flat" },
					{ event:"onAccelerometerShake", label:"Shake" },
				],
			}),
			SliderRow({ 
				name: "X",
				event: "onAccelerometerXChanged",
				label: "     X",
				min: -100,
				max: 100,
				step: 1,
				unit: "%",
				value: 0
			}),
			SliderRow({ 
				name: "Y",
				event: "onAccelerometerYChanged",
				label: "     Y",
				min: -100,
				max: 100,
				step: 1,
				unit: "%",
				value: 0
			}),
			SliderRow({ 
				name: "Z",
				event: "onAccelerometerZChanged",
				label: "     Z",
				min: -100,
				max: 100,
				step: 1,
				unit: "%",
				value: 100
			}),
			TimerRow({ 
				name: "SLEEP_TIMER",
				label: "Sleep",
				interval: 100,
				tick: "onSleepTimerChanged",
				event: "onSleepTimerFinished",
			}),
			InfoRow({
				name: "SLEEP_RETAIN",
				label: "     Retain",
				value: "",
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
				Content($, { Behavior: LEDBehavior, left:0, top:0, skin:ledSkin, visible:false }),
				Content($, { skin: { texture:{ base, path:"moddable_four/0.png" }, x:0, y:0, width:260, height:340 } }),
				DeviceScreen($, { left:70, width:128, top:60, height:128, rotation:0 }),
			],
		})),
		90: DeviceContainer.template($ => ({ 
			Behavior: MockupBehavior, 
			contents:[
				Content($, { Behavior: LEDBehavior, left:0, bottom:0, skin:ledSkin, visible:false }),
				Content($, { skin: { texture:{ base, path:"moddable_four/90.png" }, x:0, y:0, width:340, height:260 } }),
				DeviceScreen($, { left:60, width:128, top:62, height:128, rotation:90 }),
			],
		})),
		180: DeviceContainer.template($ => ({ 
			Behavior: MockupBehavior, 
			contents:[
				Content($, { Behavior: LEDBehavior, bottom:0, right:0, skin:ledSkin, visible:false }),
				Content($, { skin: { texture:{ base, path:"moddable_four/180.png" }, x:0, y:0, width:260, height:340 } }),
				DeviceScreen($, { left:62, width:128, top:152, height:128, rotation:180 }),
			],
		})),
		270: DeviceContainer.template($ => ({ 
			Behavior: MockupBehavior,
			contents:[
				Content($, { Behavior: LEDBehavior, right:0, top:0, skin:ledSkin, visible:false }),
				Content($, { skin: { texture:{ base, path:"moddable_four/270.png" }, x:0, y:0, width:340, height:260 } }),
				DeviceScreen($, { left:152, width:128, top:70, height:128, rotation:270 }),
			],
		})),
	}
};
