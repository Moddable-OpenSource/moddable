import {
	Screen,
} from "piu/Screen";

import {
	ControlsColumn,
	ButtonsRow,
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

class MockupBehavior extends DeviceBehavior {
	onAbort(container, status) {
		if (this.reloadOnAbort) {
			application.defer("doReloadFile");
			return true;
		}
	}
	onCreate(container, device) {
		super.onCreate(container, device);
		this.reloadOnAbort = false;
	}
	onAButtonDown(container) {
		this.postJSON(container, { aButton:1 });
	}
	onAButtonUp(container) {
		this.postJSON(container, { aButton:0 });
	}
	onBButtonDown(container) {
		this.postJSON(container, { bButton:1 });
	}
	onBButtonUp(container) {
		this.postJSON(container, { bButton:0 });
	}
	onCButtonDown(container) {
		this.postJSON(container, { cButton:1 });
	}
	onCButtonUp(container) {
		this.postJSON(container, { cButton:0 });
	}
	onKeyDown(container, key) {
		const f = "on" + key.toLowerCase() + "ButtonDown";
		debugger
		this[f]?.();
	}
	onKeyUp(container, key) {
		const f = "on" + key.toLowerCase() + "ButtonUp";
		this[f]?.();
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
}

export default {
	applicationName: "m5stack/debug/.*",
	title:"M5Stack",
	Workers: {},
	ControlsTemplate:ControlsColumn.template($ => ({
		contents:[
			ButtonsRow({ 
				label: "Button",
				buttons: [
					{ eventDown:"onAButtonDown", eventUp:"onAButtonUp", label:"a" },
					{ eventDown:"onBButtonDown", eventUp:"onBButtonUp", label:"b" },
					{ eventDown:"onCButtonDown", eventUp:"onCButtonUp", label:"c" },
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
				Content($, { skin: { texture:{ base, path:"m5stack/0.png" }, x:0, y:0, width:410, height:405 } }),
				DeviceScreen($, { left:46, width:320, top:79, height:240, rotation:0 }),
			],
		})),
		90: DeviceContainer.template($ => ({ 
			Behavior: MockupBehavior, 
			contents:[
				Content($, { skin: { texture:{ base, path:"m5stack/90.png" }, x:0, y:0, width:405, height:410 } }),
				DeviceScreen($, { left:82, width:240, top:41, height:320, rotation:90 }),
			],
		})),
		180: DeviceContainer.template($ => ({ 
			Behavior: MockupBehavior, 
			contents:[
				Content($, { skin: { texture:{ base, path:"m5stack/180.png" }, x:0, y:0, width:410, height:405 } }),
				DeviceScreen($, { left:44, width:320, top:86, height:240, rotation:180 }),
			],
		})),
		270: DeviceContainer.template($ => ({ 
			Behavior: MockupBehavior,
			contents:[
				Content($, { skin: { texture:{ base, path:"m5stack/270.png" }, x:0, y:0, width:405, height:410 } }),
				DeviceScreen($, { left:89, width:240, top:44, height:320, rotation:270 }),
			],
		})),
	}
};
