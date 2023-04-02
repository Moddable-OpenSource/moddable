/*---
description: 
flags: [onlyStrict]
---*/

const style = new Style({ font:"16px Open Sans", color: "black" });

const content = new Label(null, {
	top: 0, height: 50, left: 0, width: 50, style,
	skin: new Skin({ fill: "white" }),
	string: "test"
});

new Application(null, { skin: new Skin({ fill: "black" }), contents: [content] });

screen.checkImage("4510175e0a4b0c13f294d9473ddc8a7f");

content.sizeBy(50, 50);
screen.checkImage("3d84804199419ad24f057b6d0e441ade");

content.sizeBy(-25, -25);
screen.checkImage("544ca9f1332f953e647654f0d5e0df13");