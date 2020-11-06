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
const ledSkin = { texture:{ base, path:"moddable_one/led.png" }, x:0, y:0, width:160, height:160 };

class MockupBehavior extends DeviceBehavior {
	onDefaultButtonDown(container) {
		this.postJSON(container, { button:1 });
	}
	onDefaultButtonUp(container) {
		this.postJSON(container, { button:0 });
	}
	onKeyDown(container, key) {
		const code = key.charCodeAt(0);
		if (code == 127)
			this.onDefaultButtonDown(container);
	}
	onKeyUp(container, key) {
		const code = key.charCodeAt(0);
		if (code == 127)
			this.onDefaultButtonUp(container);
	}
	onJSON(container, json) {
		if ("led" in json)
			container.distribute("onLEDChanged", json.led);
	}
}

class LEDBehavior extends Behavior {
	onLEDChanged(content, state) {
		content.visible = state;
	}
}

export default {
	applicationName: "moddable_one/debug/.*",
	sortingTitle:"0001",
	title:"Moddable One",
	Workers: {
	},
	ControlsTemplate:ControlsColumn.template($ => ({
		contents:[
			ButtonsRow({ 
				label: "Button",
				buttons: [
					{ eventDown:"onDefaultButtonDown", eventUp:"onDefaultButtonUp", label:"Default" },
				],
			}),
		]
	})),
	DeviceTemplates: {
		0: DeviceContainer.template($ => ({ 
			Behavior: MockupBehavior, 
			contents:[
				Content($, { Behavior: LEDBehavior, right:0, bottom:0, skin:ledSkin, visible:false }),
				Content($, { skin: { texture:{ base, path:"moddable_one/0.png" }, x:0, y:0, width:340, height:500 } }),
				DeviceScreen($, { left:40, width:240, top:80, height:320, rotation:0 }),
			],
		})),
		90: DeviceContainer.template($ => ({ 
			Behavior: MockupBehavior, 
			contents:[
				Content($, { Behavior: LEDBehavior, right:0, top:0, skin:ledSkin, visible:false }),
				Content($, { skin: { texture:{ base, path:"moddable_one/90.png" }, x:0, y:0, width:500, height:340 } }),
				DeviceScreen($, { left:80, width:320, top:40, height:240, rotation:90 }),
			],
		})),
		180: DeviceContainer.template($ => ({ 
			Behavior: MockupBehavior, 
			contents:[
				Content($, { Behavior: LEDBehavior, left:0, top:0, skin:ledSkin, visible:false }),
				Content($, { skin: { texture:{ base, path:"moddable_one/180.png" }, x:0, y:0, width:340, height:500 } }),
				DeviceScreen($, { left:40, width:240, top:100, height:320, rotation:180 }),
			],
		})),
		270: DeviceContainer.template($ => ({ 
			Behavior: MockupBehavior,
			contents:[
				Content($, { Behavior: LEDBehavior, left:0, bottom:0, skin:ledSkin, visible:false }),
				Content($, { skin: { texture:{ base, path:"moddable_one/270.png" }, x:0, y:0, width:500, height:340 } }),
				DeviceScreen($, { left:100, width:320, top:40, height:240, rotation:270 }),
			],
		})),
	}
};
