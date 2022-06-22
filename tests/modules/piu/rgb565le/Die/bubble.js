/*---
description: 
flags: [onlyStrict]
---*/

const colorSkin = new Skin({  fill: ["red", "yellow"] });

class DieBehavior extends Behavior {
	onDisplaying(die) {
		die.fill()
			.cut();
	}
}

class StateChangeBehavior extends DieBehavior {
	changeState(content, state) {
		content.state = state;
	}	
}

const content = new Die(null, {
	height: 50, width: 50, skin: colorSkin,
	Behavior: StateChangeBehavior
});

const container = new Die(null, {
	height: 100, width: 100, skin: colorSkin,
	contents: [ content ],
	Behavior: DieBehavior
});

const container2 = new Die(null, {
	height: 200, width: 200, skin: colorSkin,
	contents: [ container ],
	Behavior: StateChangeBehavior
});

new Application(null, {
	skin: colorSkin,
	contents: [ container2 ]
});

screen.checkImage("73328e0ea82021eb0cef0ac14a7832ee");

content.bubble("changeState", 1);
screen.checkImage("7c077be88252560e30f771cb95a772fd");

content.bubble("changeState", 0.75);
screen.checkImage("f5d65e22b3ae02ee408c2583c361c4db");