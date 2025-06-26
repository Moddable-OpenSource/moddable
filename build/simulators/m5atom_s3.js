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
	onKeyDown(container, key) {
		const f = "on" + key.toUpperCase() + "ButtonDown";
		this[f]?.();
	}
	onKeyUp(container, key) {
		const f = "on" + key.toUpperCase() + "ButtonUp";
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
	applicationName: "m5atom_s3/debug/.*",
	title:"M5Atom S3",
	Workers: {},
	ControlsTemplate:ControlsColumn.template($ => ({
		contents:[
			ButtonsRow({ 
				label: "Button",
				buttons: [
					{ eventDown:"onAButtonDown", eventUp:"onAButtonUp", label:"a" }
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
				Content($, { skin: { texture:{ base, path:"m5atom_s3/0.png" }, x:0, y:0, width:240, height:240 } }),
				DeviceScreen($, { left:57, width:128, top:56, height:128, rotation:0 }),
			],
		})),
		90: DeviceContainer.template($ => ({ 
			Behavior: MockupBehavior, 
			contents:[
				Content($, { skin: { texture:{ base, path:"m5atom_s3/90.png" }, x:0, y:0, width:240, height:240 } }),
				DeviceScreen($, { left:57, width:128, top:56, height:128, rotation:90 }),
			],
		})),
		180: DeviceContainer.template($ => ({ 
			Behavior: MockupBehavior, 
			contents:[
				Content($, { skin: { texture:{ base, path:"m5atom_s3/180.png" }, x:0, y:0, width:240, height:240 } }),
				DeviceScreen($, { left:57, width:128, top:56, height:128, rotation:180 }),
			],
		})),
		270: DeviceContainer.template($ => ({ 
			Behavior: MockupBehavior,
			contents:[
				Content($, { skin: { texture:{ base, path:"m5atom_s3/270.png" }, x:0, y:0, width:240, height:240 } }),
				DeviceScreen($, { left:57, width:128, top:55, height:128, rotation:270 }),
			],
		})),
	}
};
