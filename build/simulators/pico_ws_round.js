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

class MockupBehavior extends DeviceBehavior {
	onAbort(container, status) {
		if (this.reloadOnAbort) {
			application.defer("doReloadFile");
			return true;
		}
	}
	onConfigure(container) {
	}
	onCreate(container, device) {
		super.onCreate(container, device);
		container.duration = 0;
		container.interval = 250;
	}
	onDefaultButtonDown(container) {
		this.postJSON(container, { button:1 });
	}
	onDefaultButtonUp(container) {
		this.postJSON(container, { button:0 });
	}
	onFinished(container) {
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
	onTimeChanged(container) {
	}
}

class LEDBehavior extends Behavior {
	onLEDChanged(content, state) {
		content.visible = state;
	}
}

export default {
	applicationName: "pico_ws_round.*",
	title:"WaveShare Pico RP2040",
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
				Content($, { skin: { texture:{ base, path:"./pico_ws_round/0.png" }, x:0, y:0, width:290, height:310 } }),
				DeviceScreen($, { left:25, width:240, top:25, height:240, rotation:0, circular:true }),
			],
		})),
		90: DeviceContainer.template($ => ({ 
			Behavior: MockupBehavior, 
			contents:[
				Content($, { skin: { texture:{ base, path:"./pico_ws_round/90.png" }, x:0, y:0, width:310, height:290 } }),
				DeviceScreen($, { left:25, width:240, top:25, height:240, rotation:90, circular:true }),
			],
		})),
		180: DeviceContainer.template($ => ({ 
			Behavior: MockupBehavior, 
			contents:[
				Content($, { skin: { texture:{ base, path:"./pico_ws_round/180.png" }, x:0, y:0, width:290, height:310 } }),
				DeviceScreen($, { left:25, width:240, top:45, height:240, rotation:180, circular:true }),
			],
		})),
		270: DeviceContainer.template($ => ({ 
			Behavior: MockupBehavior,
			contents:[
				Content($, { skin: { texture:{ base, path:"./pico_ws_round/270.png" }, x:0, y:0, width:310, height:290 } }),
				DeviceScreen($, { left:45, width:240, top:25, height:240, rotation:270, circular:true }),
			],
		})),
	}
};
