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

screen.checkImage("eee4cd48a70a40ed042fb9092852f98a");

content.bubble("changeState", 1);
screen.checkImage("4c4563403474c8041a9fa91380761fb0");

content.bubble("changeState", 0.75);
screen.checkImage("e3b93789511484099b6ae9a1d33c64c7");