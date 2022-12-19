/*---
description: 
flags: [onlyStrict]
---*/

const ContentTemplate = Scroller.template($ => ({
	width: 80, height: 80,
	skin: new Skin({ fill: $ })
}));

const content = new ContentTemplate("red", { left: 20, top: 20 });

new Application(null, { skin: new Skin({ fill: "white" }), contents: [content] });

screen.checkImage("401fcd361c70270be68a69bff41d777d");