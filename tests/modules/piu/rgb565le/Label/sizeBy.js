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

content.sizeBy(50, 50);
screen.checkImage("c64616a23fddc5f981ded5a40c667bb3");

content.sizeBy(-25, -25);
screen.checkImage("e016f343772137dae570328a8aad3460");