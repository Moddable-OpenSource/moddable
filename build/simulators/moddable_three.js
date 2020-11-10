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
const ledSkin = { texture:{ base, path:"moddable_three/led.png" }, x:0, y:0, width:160, height:160 };

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
	applicationName: "moddable_three/debug/.*",
	sortingTitle:"0003",
	title:"Moddable Three",
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
				Content($, { skin: { texture:{ base, path:"moddable_three/0.png" }, x:0, y:0, width:410, height:220 } }),
				DeviceScreen($, { left:70, width:250, top:50, height:122, rotation:0 }),
			],
		})),
		90: DeviceContainer.template($ => ({ 
			Behavior: MockupBehavior, 
			contents:[
				Content($, { Behavior: LEDBehavior, right:0, top:0, skin:ledSkin, visible:false }),
				Content($, { skin: { texture:{ base, path:"moddable_three/90.png" }, x:0, y:0, width:220, height:410 } }),
				DeviceScreen($, { left:50, width:122, top:90, height:250, rotation:90 }),
			],
		})),
		180: DeviceContainer.template($ => ({ 
			Behavior: MockupBehavior, 
			contents:[
				Content($, { Behavior: LEDBehavior, left:0, top:0, skin:ledSkin, visible:false }),
				Content($, { skin: { texture:{ base, path:"moddable_three/180.png" }, x:0, y:0, width:410, height:220 } }),
				DeviceScreen($, { left:90, width:250, top:48, height:122, rotation:180 }),
			],
		})),
		270: DeviceContainer.template($ => ({ 
			Behavior: MockupBehavior,
			contents:[
				Content($, { Behavior: LEDBehavior, left:0, bottom:0, skin:ledSkin, visible:false }),
				Content($, { skin: { texture:{ base, path:"moddable_three/270.png" }, x:0, y:0, width:220, height:410 } }),
				DeviceScreen($, { left:48, width:122, top:70, height:250, rotation:270 }),
			],
		})),
	}
};
