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

screen.checkImage("73328e0ea82021eb0cef0ac14a7832ee");
application.distribute("changeState", 1);
screen.checkImage("3f60942166d96e84ed78b1941f108926");
application.distribute("changeState", 0.75);
screen.checkImage("337eb6778402eb4008dab9016c134bb9");
