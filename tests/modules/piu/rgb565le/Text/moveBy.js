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

content.moveBy(50, 50);
screen.checkImage("76aa847da159ee6804a880803b393943");

content.moveBy(-25, -25);
screen.checkImage("29084dde3c3bc14d690d587c1f55dda4");