/*---
description: 
flags: [onlyStrict]
---*/

const colorSkin = new Skin({ fill: ["red", "yellow"] });

class StateChangeBehavior {
	changeState(content, state) {
		content.state = state;
	}	
}

const content = new Container(null, {
	top: 0, left: 0, height: 80, width: 80, 
	skin: colorSkin, Behavior: StateChangeBehavior
});

new Application(null, {
	skin: new Skin({ fill: "white" }),
	contents: [ content ]
});

screen.checkImage("985606a9e36a608e7d51f25b829a08a8");

content.delegate("changeState", 1);
screen.checkImage("1d5d8720d0cc3b88842b2eaace2f45f9");

content.delegate("changeState", 0.75);
screen.checkImage("c5472ea529d8988943ccdfc3c5fbe7b5");