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
	right: 20, left: 20, top: 20, bottom: 180, 
	skin: colorSkin, Behavior: StateChangeBehavior 
});

const content2 = new Content(null, { 
	right: 20, left: 20, top: 180, bottom: 20, 
	skin: colorSkin
});

new Application(null, {
	skin: colorSkin,
	contents: [ content, content2 ],
	Behavior: StateChangeBehavior
});

screen.checkImage("73d09edd8afd47d9038203b6c975f44c");
application.distribute("changeState", 1);
screen.checkImage("629c607faf060721f75196b5344fe974");
application.distribute("changeState", 0.75);
screen.checkImage("f2e08a7351408194550682bae2e47340");
