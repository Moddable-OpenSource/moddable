/*---
description: 
flags: [onlyStrict]
---*/

const style = new Style({ font:"16px Open Sans", color: "black" });

const content = new Text(null, {
	top: 0, height: 50, left: 0, width: 50, style,
	skin: new Skin({ fill: "white" }),
	string: "test"
});

new Application(null, { skin: new Skin({ fill: "black" }), contents: [content] });

screen.checkImage("f5540d10271ed6c15cf51a7b1e0c16ff");

content.moveBy(50, 50);
screen.checkImage("3ceed4ee6769a53fad4eaee3ca4941b3");

content.moveBy(-25, -25);
screen.checkImage("c5c95447d5bfb7fffe10821f1a0f7230");