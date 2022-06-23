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

screen.checkImage("71a0212a51d4865fccc2bc3f38e2f305");

content.delegate("changeWidth", 100);
screen.checkImage("4324fba1206d67ecb4cb6bed8c674818");
