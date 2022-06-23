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

const content = new Layout(null, {
	height: 50, width: 50, skin: colorSkin
});

const container = new Layout(null, {
	height: 100, width: 100, skin: colorSkin,
	contents: [ content ],
	Behavior: StateChangeBehavior
});

const container2 = new Layout(null, {
	height: 200, width: 200, skin: colorSkin,
	contents: [ container ]
});

new Application(null, {
	skin: colorSkin,
	contents: [ container2 ],
	Behavior: StateChangeBehavior
});

screen.checkImage("73328e0ea82021eb0cef0ac14a7832ee");

content.bubble("changeState", 1);
screen.checkImage("0cf0ec78c2863cfac32c5ca0d838298e");

content.bubble("changeState", 0.75);
screen.checkImage("131667dd75e3bbd0978812e089990087");