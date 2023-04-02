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

screen.checkImage("8994063933f51e96bfff46f972dcc11f");

content.bubble("changeState", 1);
screen.checkImage("81d0ab7022ee632d82d1752d4e2844cd");

content.bubble("changeState", 0.75);
screen.checkImage("2ad3d3c0a70347d3817b49dd31c707dd");