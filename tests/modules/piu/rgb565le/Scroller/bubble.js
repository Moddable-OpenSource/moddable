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

const content = new Scroller(null, {
	height: 50, width: 50, skin: colorSkin
});

const container = new Scroller(null, {
	height: 100, width: 100, skin: colorSkin,
	contents: [ content ],
	Behavior: StateChangeBehavior
});

const container2 = new Scroller(null, {
	height: 200, width: 200, skin: colorSkin,
	contents: [ container ]
});

new Application(null, {
	skin: colorSkin,
	contents: [ container2 ],
	Behavior: StateChangeBehavior
});

screen.checkImage("73d09edd8afd47d9038203b6c975f44c");

content.bubble("changeState", 1);
screen.checkImage("5413d42293b896acf7c00893d2ffe443");

content.bubble("changeState", 0.75);
screen.checkImage("da9f54f45ff984b58972e8538814552c");