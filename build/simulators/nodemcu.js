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
const ledSkin = { texture:{ base, path:"nodemcu/led.png" }, x:0, y:0, width:160, height:190, variants:160 };

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
			container.distribute("onLEDChanged", json);
	}
}

class LEDBehavior extends Behavior {
	onLEDChanged(content, json) {
		if (content.variant == json.variant)
			content.visible = json.led;
	}
}

export default {
	applicationName: "nodemcu/debug/.*",
	title:"NodeMcu",
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
				Content($, { skin: { texture:{ base, path:"nodemcu/0.png" }, x:0, y:0, width:320, height:190 } }),
				Content($, { Behavior: LEDBehavior, left:0, top:0, skin:ledSkin, variant:0, visible:false }),
				Content($, { Behavior: LEDBehavior, left:160, top:0, skin:ledSkin, variant:1, visible:false }),
				DeviceScreen($, { left:0, width:0, top:0, height:0, rotation:0 }),
			],
		})),
	}
};
