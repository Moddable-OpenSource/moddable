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

screen.checkImage("a175b9195b15e387fa4bef51df6e0baa");

content.moveBy(50, 50);
screen.checkImage("216531a56b34b035eed8a7fc75f87c9f");

content.moveBy(-25, -25);
screen.checkImage("e135ceecc4d15c2bb8b7d5a083f129e4");