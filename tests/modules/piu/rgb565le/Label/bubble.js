/*---
description: 
flags: [onlyStrict]
---*/

const style = new Style({ font:"16px Open Sans", color: "black" });
const colorSkin = new Skin({  fill: ["red", "yellow"] });

class StateChangeBehavior {
	changeState(content, state) {
		content.state = state;
	}	
}

const content = new Label(null, {
	height: 50, width: 50, style,
	skin: colorSkin,
	string: "test"
});

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

screen.checkImage("4522087fcfbe0d55efca4b95d95bbddf");

content.bubble("changeState", 1);
screen.checkImage("f4b5dc3162371e8ee6e1fa953c8612af");

content.bubble("changeState", 0.75);
screen.checkImage("2640eedf78ae5517b588934160b699d2");