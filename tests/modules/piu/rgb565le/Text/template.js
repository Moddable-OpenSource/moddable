/*---
description: 
flags: [onlyStrict]
---*/

const style = new Style({ font:"16px Open Sans", color: "black" });

const ContentTemplate = Text.template($ => ({
	height: 50, width: 50, style,
	skin: new Skin({ fill: "gray" }),
	string: $
}));

const content = new ContentTemplate("test", { left: 20, top: 20 });

new Application(null, { skin: new Skin({ fill: "white" }), contents: [content] });

screen.checkImage("0233b925945b8884945c623b01a1e939");