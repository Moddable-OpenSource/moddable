/*
 * Copyright (c) 2016-2024 Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import Resource from "Resource";
import {} from "piu/MC";
import Timeline from "piu/Timeline";
import UARTServer from "uart";

const BLACK = "black"
const BLUE = "#192eab"
const GREEN = "#9cf"
const LITE = "#C0C0C0";
const RED = "#faa"
const WHITE = "white";

const applicationSkin = new Skin({ fill:BLACK });
const applicationStyle = new Style({ font:"semibold 16px Open Sans", color:WHITE, horizontal:"left", left:5 });

const fieldSkin = new Skin({ stroke:LITE, left:1, right:1, bottom:1 });
const fieldStyle = new Style({ font:"bold 32px Hyperspace", color:[WHITE,WHITE,RED,GREEN], bottom:7, horizontal:"center" });

const glyphsSkin = new Skin({ texture:new Texture("glyphs.png"), width:30, height:30, variants:30, color:WHITE });
const DOWN = 0;
const UP = 1;
const REFRESH = 2;
const START = 3;
const STOP = 4;
const BLUETOOTH = 5;

const popupSkin = new Skin({ fill:["#00000000", "#00000080"] });
const rowSkin = new Skin({ fill:[BLACK,BLUE] });
const rowStyle = new Style({ color:WHITE });
const frameSkin = new Skin({ fill:[BLACK,BLUE], stroke:LITE, left:1, right:1, top:1, bottom:1 });

const model = {
	attackers: [
		{ name:"All Zeros", index:0, resource:"AllZeros.js" },
		{ name:"Counter", index:1, resource:"Counter.js" },
		{ name:"Timing Channel Frustrated", index:2, resource:"TimingChannel.js" },
		{ name:"Timing Channel Exposed", index:3, resource:"TimingChannel.js", now:true },
		{ name:"User Script", index:4,  },
	],
	attackerIndex: 0,
};

const delayMS = (count) => {
	const when = Date.now() + count;
	while (Date.now() < when) {}
};

const guess = (guessedCode) => {
	guessedCode = `${guessedCode}`;

	model.GUESS.string = guessedCode;
	const secretCode = model.SECRET.string;

	for (let i = 0; i < 10; i++) {
		if (secretCode.slice(i, i + 1) !== guessedCode.slice(i, i + 1)) {
			return false;
		}
		delayMS(10);
	};
	
	model.GUESS.state = 3;
	application.behavior.onStop(application);
	return true;
};

function* generateError(error) {
	guess(error);
	yield;
}

class ApplicationBehavior extends Behavior {
	onConnected(application) {
		this.connected = true;
		model.BLUETOOTH.visible = true;
	}
	onDisconnected(application) {
		this.connected = false;
		model.BLUETOOTH.visible = false;
	}
	onDisplaying(application) {
		this.script = `
function* generateUndefined() {
	guess("UNDEFINED");
	yield;
}
`;
		this.onRefresh(application);
		this.connected = false;
		this.server = new UARTServer(application);
	}
	onReceived(application, script) {
		this.script = script;
		application.last.delegate("select", 4);
		this.onStart(application);
	}
	onRefresh(application) {
		const array = new Uint32Array(10);
		const characters = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
		let secretCode = "";
		while (secretCode.length < 10) {
			let character = characters.charAt(Math.round(Math.random() * 35));
			if (secretCode.indexOf(character) < 0)
				secretCode += character;
		}
		model.SECRET.string = secretCode;
		this.onStart(application);
	}
	onSelected(application, index) {
		this.onStart(application);
	}
	onStop(application) {
		application.stop();
		model.RUN.variant = START;
	}
	onStart(application) {
		application.stop();
	
		const data = this.data;
		
		const attacker = model.attackers[model.attackerIndex];
		let script;
		if (attacker.resource) {
			const resource = new Resource(attacker.resource);
			script = String.fromArrayBuffer(resource);
		}
		else {
			script = this.script;
		}
		const globals = { guess };
		if (attacker.now)
			globals.Date = Date;
		const compartment = new Compartment({ globals });
		try {
			const generatorFunction = compartment.evaluate(`(${script})`);
			this.generator = generatorFunction();
		}
		catch (e) {
			this.generator = null;
 			guess("ERROR");
			this.server.transmit(e.toString());
		}
		
		application.start();
		model.GUESS.state = 2;
		model.RUN.variant = STOP;
	}
	onTimeChanged(application) {
		if (this.generator) {
			try {
				if (this.generator.next().done) {
					this.onStop(application);
				}
			}
			catch (e) {
				this.generator = null;
				guess("ERROR");
				this.server.transmit(e.toString());
			}
		}
	}
}

class ButtonBehavior extends Behavior {
	onCreate(application, data) {
		this.data = data;
    }
	onTap(container) {
		debugger
	}
	onTouchBegan(container, id, x, y, ticks) {
		container.captureTouch(id, x, y, ticks);
		container.first.state = 1;
	}
	onTouchEnded(container) {
		container.first.state = 0;
		this.onTap(container);
	}
}

class RefreshButtonBehavior extends ButtonBehavior {
	onTap(container) {
		container.bubble("onRefresh");
	}
}

class RunButtonBehavior extends ButtonBehavior {
	onTap(container) {
		if (container.first.first.variant == START)
			container.bubble("onStart");
		else
			container.bubble("onStop");
	}
}

class AttackerButtonBehavior extends ButtonBehavior {
	onCreate(application, data) {
		this.data = data;
		this.expanded = false;
    }
	onFinished(container) {
		if (this.expanded) {
			this.expanded = false;
			container.height = 80; 
			container.state = 0;
			container.distribute("onSelecting", -1);
		}
		else {
			this.expanded = true;
		}
	}
	onSelected(container, index) {
		this.data.attackerIndex = index;
		this.onTap(container);
	}
	onTimeChanged(container) {
		this.timeline.seekTo(container.time);
	}
	onTouchBegan(container, id, x, y, ticks) {
		container.captureTouch(id, x, y, ticks);
		container.first.state = 1;
	}
	onTap(container) {
		const body = container.last;
		const menu = body.first;
		const item = menu.content(this.data.attackerIndex);

		const duration = 250;
		const timeline = new Timeline();
		if (this.expanded) {
			container.first.first.variant = DOWN; 
			timeline.to(body, { height:40 }, duration, Math.quadEaseOut, 0);
			timeline.to(menu, { y:container.y + 40 - (this.data.attackerIndex * 40) }, duration, Math.quadEaseOut, -duration);
			timeline.to(container, { state:0}, duration, Math.quadEaseOut, 0);
			timeline.to(container, { item:0}, duration, Math.quadEaseOut, -duration);
		}
		else {
			container.first.first.variant = UP; 
			container.height = application.height; 
			container.state = 1;
			container.distribute("onSelecting", this.data.attackerIndex);
			timeline.to(body, { height:menu.height }, duration, Math.quadEaseOut, 0);
			timeline.to(menu, { y:container.y + 40 }, duration, Math.quadEaseOut, -duration);
			timeline.to(container, { state:1 }, duration, Math.quadEaseOut, -duration);
		}
		this.timeline = timeline;
		container.duration = timeline.duration;
		container.time = 0;
		container.start();
	}
	select(container, index) {
		if (this.expanded) {
			container.distribute("onSelecting", index);
			this.onSelected(container, index);
		}
		else {
			this.data.attackerIndex = index;
			const body = container.last;
			const menu = body.first;
			menu.y = container.y + 40 - (index * 40);
		}
	}
}

class AttackerRowBehavior extends Behavior {
	onCreate(application, data) {
		this.data = data;
    }
	onSelecting(row, index) {
		row.active = index >= 0;
		row.state = (this.data.index == index) ? 1 : 0;
	}
	onTouchBegan(row, id, x, y, ticks) {
		row.captureTouch(id, x, y, ticks);
		row.container.distribute("onSelecting", this.data.index);
	}
	onTouchEnded(row) {
		row.bubble("onSelected", this.data.index);
	}
}

const AttackerRow = Row.template($ => ({
	left:0, right:0, height:40, skin:rowSkin, style:rowStyle, Behavior:AttackerRowBehavior,
	contents: [
		Label($, { left:0, right:0, height:40, string:$.name }),
	]
}));

const ChallengeApplication = Application.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:applicationSkin, style:applicationStyle, Behavior:ApplicationBehavior,
	contents: [
		Container($, {
			left:0, right:0, height:80, active:true, Behavior:RefreshButtonBehavior,
			contents: [
				Row($, {
					left:10, right:10, top:0, height:40, skin:frameSkin,
					contents: [
						Content($, { left:5, skin:glyphsSkin, variant:REFRESH, state:0 }),
						Label($, { left:0, right:0, top:0, bottom:0, string:"Secret" }),
					]
				}),
				Label($, { anchor:"SECRET", left:10, right:10, top:40, height:40, skin:fieldSkin, style:fieldStyle }),
			]
		}),
		Container($, {
			left:0, right:0, height:80, bottom:20, active:true, Behavior:RunButtonBehavior,
			contents: [
				Row($, {
					left:10, right:10, top:0, height:40, skin:frameSkin,
					contents: [
						Content($, { anchor:"RUN", left:5, skin:glyphsSkin, variant:START, state:0 }),
						Label($, { left:0, right:0, top:0, bottom:0, string:"Guess" }),
					]
				}),
				Label($, { anchor:"GUESS", left:10, right:10, top:40, height:40, skin:fieldSkin, style:fieldStyle, state:2, string:"0000000000" }),
			]
		}),
		Container($, {
			left:0, right:0, top:20, height:80, skin:popupSkin, state:0, active:true, Behavior:AttackerButtonBehavior,
			contents: [
				Row($, {
					left:10, right:10, top:0, height:40, skin:frameSkin,
					contents: [
						Content($, { left:5, skin:glyphsSkin, variant:DOWN, state:0 }),
						Label($, { left:0, right:0, top:0, bottom:0, string:"Attacker" }),
						Content($, { anchor:"BLUETOOTH", right:5, skin:glyphsSkin, variant:BLUETOOTH, visible:false }),
					]
				}),
				Container($, {
					left:10, right:10, top:40, height:40, clip:true,
					contents: [
						Column($, {
							left:0, right:0, top:0,
							contents: $.attackers.map($$ => new AttackerRow($$)),
						}),
						Content($, { left:0, right:0, top:0, bottom:0, skin:fieldSkin }),
					]
				}),
			]
		}),
	]
}));

export default new ChallengeApplication(model, { 
	commandListLength: 8192,
	displayListLength: 8192,
	touchCount:1 
});
