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

const content = new Text(null, {
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

screen.checkImage("5e12be1ae9b54586c920d01a9a925a30");

content.bubble("changeState", 1);
screen.checkImage("d50d15464da2c0168447c1539d511207");

content.bubble("changeState", 0.75);
screen.checkImage("ef2a8ae1de643a20304bfaa980d88ae8");