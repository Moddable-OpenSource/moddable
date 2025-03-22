import {
	Screen,
} from "piu/Screen";

import {
	getBackgroundComponent,
} from "assets";

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
const ledTexture = new Texture({ base, path:"moddable_six/led.png" });

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
		if ("backlight" in json)
			container.last.transparency = ((100 - json.backlight) / 100) * 0.9;
		else if ("led" in json)
			container.distribute("onLEDChanged", json.led);
		else if ("xsbug" in json) {
			if (json.xsbug == "abort")
				application.defer("doReloadFile");
		}
	}
	onReloadOnAbortChanged(container, data) {
		this.reloadOnAbort = data.value;
	}
}

function RGBtoHSV(r, g, b) {
    if (arguments.length === 1) {
        g = r.g, b = r.b, r = r.r;
    }
    var max = Math.max(r, g, b), min = Math.min(r, g, b),
        d = max - min,
        h,
        s = (max === 0 ? 0 : d / max),
        v = max / 255;

    switch (max) {
        case min: h = 0; break;
        case r: h = (g - b) + d * (g < b ? 6: 0); h /= 6 * d; break;
        case g: h = (b - r) + d * 2; h /= 6 * d; break;
        case b: h = (r - g) + d * 4; h /= 6 * d; break;
    }

    return {
        h: h,
        s: s,
        v: v
    };
}

function HSVtoRGB(h, s, v) {
    var r, g, b, i, f, p, q, t;
    if (arguments.length === 1) {
        s = h.s, v = h.v, h = h.h;
    }
    i = Math.floor(h * 6);
    f = h * 6 - i;
    p = v * (1 - s);
    q = v * (1 - f * s);
    t = v * (1 - (1 - f) * s);
    switch (i % 6) {
        case 0: r = v, g = t, b = p; break;
        case 1: r = q, g = v, b = p; break;
        case 2: r = p, g = v, b = t; break;
        case 3: r = p, g = q, b = v; break;
        case 4: r = t, g = p, b = v; break;
        case 5: r = v, g = p, b = q; break;
    }
    return {
        r: Math.round(r * 255),
        g: Math.round(g * 255),
        b: Math.round(b * 255)
    };
}

class LEDBehavior extends Behavior {
	onLEDChanged(port, color) {
		this.color = color;
		port.invalidate();
	}
	onDraw(port) {
		const color = this.color;
		const c = getBackgroundComponent();
		let r = (color & 0xFF0000) >> 16;
		let g = (color & 0x00FF00) >> 8;
		let b = (color & 0x0000FF);
		let a = 1;
		const hsv = RGBtoHSV(r, g, b);
		a = hsv.v;
		hsv.v = 1;
		const rgb = HSVtoRGB(hsv);
		r = rgb.r;
		g = rgb.g;
		b = rgb.b;
		port.drawTexture(ledTexture, rgba(r, g, b, a), 0, 0, 0, 0, port.width, port.height);
	}
}

export default {
	applicationName: "moddable_six/debug/.*",
	sortingTitle:"0005",
	title:"Moddable Six",
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
				Port($, { Behavior: LEDBehavior, right:0, bottom:0, width:200, height:200, visible:true }),
				Content($, { skin: { texture:{ base, path:"moddable_six/0.png" }, x:0, y:0, width:390, height:580 } }),
				DeviceScreen($, { left:74, width:240, top:127, height:320, rotation:0 }),
			],
		})),
		90: DeviceContainer.template($ => ({ 
			Behavior: MockupBehavior, 
			contents:[
				Port($, { Behavior: LEDBehavior, right:0, top:0, width:200, height:200, visible:true }),
				Content($, { skin: { texture:{ base, path:"moddable_six/90.png" }, x:0, y:0, width:580, height:390 } }),
				DeviceScreen($, { left:127, width:320, top:76, height:240, rotation:90 }),
			],
		})),
		180: DeviceContainer.template($ => ({ 
			Behavior: MockupBehavior, 
			contents:[
				Port($, { Behavior: LEDBehavior, left:0, top:0, width:200, height:200, visible:true }),
				Content($, { skin: { texture:{ base, path:"moddable_six/180.png" }, x:0, y:0, width:390, height:580 } }),
				DeviceScreen($, { left:76, width:240, top:133, height:320, rotation:180 }),
			],
		})),
		270: DeviceContainer.template($ => ({ 
			Behavior: MockupBehavior,
			contents:[
				Port($, { Behavior: LEDBehavior, left:0, bottom:0, width:200, height:200, visible:true }),
				Content($, { skin: { texture:{ base, path:"moddable_six/270.png" }, x:0, y:0, width:580, height:390 } }),
				DeviceScreen($, { left:133, width:320, top:74, height:240, rotation:270 }),
			],
		})),
	}
};
