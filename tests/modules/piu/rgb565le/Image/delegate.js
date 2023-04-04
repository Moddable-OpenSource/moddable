/*---
description: 
flags: [onlyStrict]
---*/

class WidthChangeBehavior {
	changeWidth(content, width) {
		content.width = width;
	}	
}

const content = new Image(null, ({
	left: 0, width: 240, path: "screen2.cs",
	Behavior: WidthChangeBehavior
}));

new Application(null, {
	skin: new Skin({ fill: "white" }),
	contents: [ content ]
});

screen.checkImage("6976b2da2767b4453c4def3163003888");

content.delegate("changeWidth", 100);
screen.checkImage("ced20a618e858c92117ebd10cbdca063");
