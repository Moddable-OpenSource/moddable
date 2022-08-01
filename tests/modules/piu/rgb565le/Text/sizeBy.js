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

screen.checkImage("1f23cbda3d4ad35b78d11ad8ec7d5792");

content.sizeBy(50, 50);
screen.checkImage("b17c7ae5af32c1e98e254b1e65c5a89e");

content.sizeBy(-25, -25);
screen.checkImage("4e1574343b41f55809ee82b05eba3657");