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
	right: 20, left: 20, top: 20, bottom: 70, 
	skin: colorSkin, Behavior: StateChangeBehavior 
});

const content2 = new Content(null, { 
	right: 20, left: 20, top: 70, bottom: 20, 
	skin: colorSkin
});

const container = new Container(null, {
	top: 0, height: 100, left: 0, width: 100,
	skin: colorSkin, Behavior: StateChangeBehavior,
	contents: [ content, content2 ]
});

new Application(null, {
	skin: new Skin({  fill: "white" }),
	contents: [ container ],
	Behavior: StateChangeBehavior
});

screen.checkImage("db4dc3d8246223a45feb8ee57ac5117d");
container.distribute("changeState", 1);
screen.checkImage("e607338fc1cfa1ca820e1923bfb88484");
container.distribute("changeState", 0.75);
screen.checkImage("e4e6c0c275fd03fe1731c5d385e5d316");