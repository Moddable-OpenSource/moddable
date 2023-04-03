/*---
description: 
flags: [onlyStrict]
---*/

const ContentTemplate = Port.template($ => ({
	width: 80, height: 80,
	skin: new Skin({ fill: $ })
}));

const content = new ContentTemplate("red", { left: 20, top: 20 });

new Application(null, { skin: new Skin({ fill: "white" }), contents: [content] });

screen.checkImage("fcd8fdc85fe4e1d6f88702a5bfa58dbf");