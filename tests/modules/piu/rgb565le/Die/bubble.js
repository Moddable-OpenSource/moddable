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

screen.checkImage("73d09edd8afd47d9038203b6c975f44c");

content.bubble("changeState", 1);
screen.checkImage("55c671d11472f9ca50588996859007fa");

content.bubble("changeState", 0.75);
screen.checkImage("80309e1395654656c5fdbd4b10d52085");