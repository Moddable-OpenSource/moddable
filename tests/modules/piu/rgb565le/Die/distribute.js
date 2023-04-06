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

const container = new Die(null, {
	top: 0, height: 100, left: 0, width: 100,
	skin: colorSkin,
	contents: [ content, content2 ],
	Behavior: class extends StateChangeBehavior {
		onDisplaying(die) {
			die.fill()
				.cut();
		}
	}
});

new Application(null, {
	skin: new Skin({  fill: "white" }),
	contents: [ container ],
	Behavior: StateChangeBehavior
});

screen.checkImage("f30e02a81ef52d72e5e3bfcbeaa5d913");
container.distribute("changeState", 1);
screen.checkImage("076b1bcd69cd0d1388abcec8250aacc9");
container.distribute("changeState", 0.75);
screen.checkImage("5dd6c80745721ea1eb1e8b4ae2c2f513");