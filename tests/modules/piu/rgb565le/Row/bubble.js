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

const content = new Row(null, {
	height: 50, width: 50, skin: colorSkin
});

const container = new Row(null, {
	height: 100, width: 100, skin: colorSkin,
	contents: [ content ],
	Behavior: StateChangeBehavior
});

const container2 = new Row(null, {
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
screen.checkImage("d4b516eedf1f684f5832e8960da4aec8");

content.bubble("changeState", 0.75);
screen.checkImage("a6273004687f5d8a931e3a171ac504ff");