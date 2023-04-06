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

const content = new Content(null, { 
	right: 20, left: 20, top: 20, bottom: 20, 
	skin: colorSkin, Behavior: StateChangeBehavior 
});

const content2 = new Content(null, { 
	right: 20, left: 20, top: 20, bottom: 20, 
	skin: colorSkin
});

const container = new Row(null, {
	top: 0, height: 100, left: 0, width: 100,
	skin: colorSkin, Behavior: StateChangeBehavior,
	contents: [ content, content2 ]
});

new Application(null, {
	skin: new Skin({  fill: "white" }),
	contents: [ container ],
	Behavior: StateChangeBehavior
});

screen.checkImage("f30e02a81ef52d72e5e3bfcbeaa5d913");
container.distribute("changeState", 1);
screen.checkImage("9f2424d6eb117dd361031e8ccbcb82e8");
container.distribute("changeState", 0.75);
screen.checkImage("13177a0764fc846987588e457b7b96cd");