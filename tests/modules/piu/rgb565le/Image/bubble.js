/*---
description: 
flags: [onlyStrict]
---*/

const colorSkin = new Skin({  fill: ["red", "yellow"] });

class StateChangeBehavior {
	changeState(content, state) {
		content.state = state;
	}	
}

const content = new Image(null, ({
	path: "color-dots-20.cs"
}));

const container = new Container(null, {
	height: 100, width: 100, skin: colorSkin,
	contents: [ content ],
	Behavior: StateChangeBehavior
});

const container2 = new Container(null, {
	height: 200, width: 200, skin: colorSkin,
	contents: [ container ]
});

new Application(null, {
	skin: colorSkin,
	contents: [ container2 ],
	Behavior: StateChangeBehavior
});

screen.checkImage("406ea991148e87a7e4683f8ace8077a5");

content.bubble("changeState", 1);
screen.checkImage("e0c6ac458cfa36311c3e56b518aeb1d0");

content.bubble("changeState", 0.75);
screen.checkImage("44158798b69a82f36c5b4d646d0d34b3");