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
const ledSkin = { texture:{ base, path:"moddable_four/led.png" }, x:0, y:0, width:160, height:160 };

class MockupBehavior extends DeviceBehavior {
	onAbort(container, status) {
		if (this.reloadOnAbort) {
			application.defer("doReloadFile");
			return true;
		}
	}
	onReloadOnAbortChanged(container, data) {
		this.reloadOnAbort = data.value;
	}
	onCreate(container, device) {
		super.onCreate(container, device);
		container.duration = 0;
		container.interval = 250;
		this.push = 0;
		this.turn = 0;
		this.reloadOnAbort = false;
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
		else if ("xsbug" in json) {
			if (json.xsbug == "abort")
				application.defer("doReloadFile");
		}
	}
	onPushCWJogDialDown(container) {
		container.start();
		this.postJSON(container, { jogdial: { push:1, turn:4 } });
		this.push = 1;
		this.turn = 1;
	}
	onPushCWJogDialUp(container) {
		container.stop();
	}
	onPushCCWJogDialDown(container) {
		container.start();
		this.postJSON(container, { jogdial: { push:1, turn:-4 } });
		this.push = 1;
		this.turn = -1;
	}
	onPushCCWJogDialUp(container) {
		container.stop();
	}
	onTimeChanged(container) {
		this.postJSON(container, { jogdial: { push:this.push, turn:this.turn }});
	}
	onTurnCWJogDialDown(container) {
		container.start();
		this.postJSON(container, { jogdial: { push:0, turn:4 } });
		this.push = 0;
		this.turn = 1;
	}
	onTurnCWJogDialUp(container) {
		container.stop();
	}
	onTurnCCWJogDialDown(container) {
		container.start();
		this.postJSON(container, { jogdial: { push:0, turn:-4 } });
		this.push = 0;
		this.turn = -1;
	}
	onTurnCCWJogDialUp(container) {
		container.stop();
	}
	onBackButtonDown(container) {
		this.postJSON(container, { button:1 });
	}
	onBackButtonUp(container) {
		this.postJSON(container, { button:0 });
	}
	onEnterButtonDown(container) {
		this.postJSON(container, { jogdial: { push:1 } });
	}
	onEnterButtonUp(container) {
		this.postJSON(container, { jogdial: { push:0 } });
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
