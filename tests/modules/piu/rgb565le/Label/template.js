/*---
description: 
flags: [onlyStrict]
---*/

const style = new Style({ font:"16px Open Sans", color: "black" });

const ContentTemplate = Label.template($ => ({
	height: 50, width: 50, style,
	skin: new Skin({ fill: "gray" }),
	string: $
}));

const content = new ContentTemplate("test", { left: 20, top: 20 });

new Application(null, { skin: new Skin({ fill: "white" }), contents: [content] });

screen.checkImage("cab2fdf449977d6b32c6d08ac339d2c9");