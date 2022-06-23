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

screen.checkImage("975239533aa81545b09931c5149584b2");

content.bubble("changeState", 1);
screen.checkImage("0aab70287d95cea35771802e16798116");

content.bubble("changeState", 0.75);
screen.checkImage("f158f6095f2407765bdd014e44874d90");