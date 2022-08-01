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

screen.checkImage("db4dc3d8246223a45feb8ee57ac5117d");
container.distribute("changeState", 1);
screen.checkImage("8a7ec08911fcde89f9a49a69cf6cfee4");
container.distribute("changeState", 0.75);
screen.checkImage("500ba32b8ad96041123b8ddd2750455f");