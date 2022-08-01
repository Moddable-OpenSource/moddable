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

screen.checkImage("73328e0ea82021eb0cef0ac14a7832ee");

content.bubble("changeState", 1);
screen.checkImage("8c4a4e8ab4ffad610730ee82800988ab");

content.bubble("changeState", 0.75);
screen.checkImage("d662b149050fd60af0d0d86f14913822");