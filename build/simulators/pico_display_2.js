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
const ledSkin = { texture:{ base, path:"pico_display_2/led.png" }, x:0, y:0, width:48, height:48 };

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
	onXButtonDown(container) {
		this.postJSON(container, { xButton:1 });
	}
	onXButtonUp(container) {
		this.postJSON(container, { xButton:0 });
	}
	onYButtonDown(container) {
		this.postJSON(container, { yButton:1 });
	}
	onYButtonUp(container) {
		this.postJSON(container, { yButton:0 });
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
		if ("led" in json)
			container.distribute("onLEDChanged", json.led);
		else
		if ("xsbug" in json) {
			if (json.xsbug == "abort")
				application.defer("doReloadFile");
		}
	}
	onReloadOnAbortChanged(container, data) {
		this.reloadOnAbort = data.value;
	}
}

class LEDBehavior extends Behavior {
	onLEDChanged(content, color) {
		if (!color.red && !color.green && !color.blue) {
			content.visible = false;
			return;
		}
		
		content.visible = true;
		content.skin = new Skin({...ledSkin, color: rgb(color.red, color.green, color.blue)});
	}
}

export default {
	applicationName: "pico_display_2/debug/.*",
	sortingTitle:"0004",
	title:"Pico Display 2 (Pimoroni)",
	Workers: {
	},
	ControlsTemplate:ControlsColumn.template($ => ({
		contents:[
			ButtonsRow({ 
				label: "Button",
				buttons: [
					{ eventDown:"onAButtonDown", eventUp:"onAButtonUp", label:"A" },
					{ eventDown:"onBButtonDown", eventUp:"onBButtonUp", label:"B" },
				],
			}),
			ButtonsRow({ 
				label: "",
				buttons: [
					{ eventDown:"onXButtonDown", eventUp:"onXButtonUp", label:"X" },
					{ eventDown:"onYButtonDown", eventUp:"onYButtonUp", label:"Y" },
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
				Content($, { skin: { texture:{ base, path:"pico_display_2/0.png" }, x:0, y:0, width:400, height:254 } }),
				Content($, { Behavior: LEDBehavior, left:360, top:105, skin:ledSkin, visible:false }),
				DeviceScreen($, { left:40, width:320, top:7, height:240, rotation:0 }),
			],
		})),
		90: DeviceContainer.template($ => ({ 
			Behavior: MockupBehavior, 
			contents:[
				Content($, { skin: { texture:{ base, path:"pico_display_2/90.png" }, x:0, y:0, width:254, height:400 } }),
				Content($, { Behavior: LEDBehavior, left:106, top:0, skin:ledSkin, visible:false }),
				DeviceScreen($, { left:7, width:240, top:45, height:320, rotation:90 }),
			],
		})),
		180: DeviceContainer.template($ => ({ 
			Behavior: MockupBehavior, 
			contents:[
				Content($, { skin: { texture:{ base, path:"pico_display_2/180.png" }, x:0, y:0, width:400, height:254 } }),
				Content($, { Behavior: LEDBehavior, left:0, top:105, skin:ledSkin, visible:false }),
				DeviceScreen($, { left:45, width:320, top:7, height:240, rotation:180 }),
			],
		})),
		270: DeviceContainer.template($ => ({ 
			Behavior: MockupBehavior,
			contents:[
				Content($, { skin: { texture:{ base, path:"pico_display_2/270.png" }, x:0, y:0, width:254, height:400 } }),
				Content($, { Behavior: LEDBehavior, left:103, top:353, skin:ledSkin, visible:false }),
				DeviceScreen($, { left:7, width:240, top:35, height:320, rotation:270 }),
			],
		})),
	}
};
