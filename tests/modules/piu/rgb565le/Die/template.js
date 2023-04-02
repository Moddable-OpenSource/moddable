/*---
description: 
flags: [onlyStrict]
---*/

const ContentTemplate = Die.template($ => ({
	width: 80, height: 80,
	skin: new Skin({ fill: $ }),
	Behavior: class extends Behavior {
		onDisplaying(die) {
			die.fill()
				.cut();
		}
	}
}));

const content = new ContentTemplate("red", { left: 20, top: 20 });

new Application(null, { skin: new Skin({ fill: "white" }), contents: [content] });

screen.checkImage("fcd8fdc85fe4e1d6f88702a5bfa58dbf");