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

screen.checkImage("73328e0ea82021eb0cef0ac14a7832ee");

content.bubble("changeState", 1);
screen.checkImage("fa6496901b2473ccb7e56591129eeee5");

content.bubble("changeState", 0.75);
screen.checkImage("78f7254ee64a612ecdf26164729806b8");