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

const content = new Column(null, {
	height: 50, width: 50, skin: colorSkin
});

const container = new Column(null, {
	height: 100, width: 100, skin: colorSkin,
	contents: [ content ],
	Behavior: StateChangeBehavior
});

const container2 = new Column(null, {
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
screen.checkImage("f4ffc4bbd902aa2a3a44cbdcb38dfb3f");

content.bubble("changeState", 0.75);
screen.checkImage("120ff95956cc113773e9fe830bff15b2");