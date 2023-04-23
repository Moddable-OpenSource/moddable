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

content.moveBy(50, 50);
screen.checkImage("071750f23b8ea022be502780a9b24e56");

content.moveBy(-25, -25);
screen.checkImage("39fffeeefb6ee505fd97f7cc40d000f3");