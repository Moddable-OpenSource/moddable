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

content.sizeBy(50, 50);
screen.checkImage("46ac774af9f1ed8ffcced9874aadccaf");

content.sizeBy(-25, -25);
screen.checkImage("79b6084ea0b2b9f601b7e9fe0efd1846");