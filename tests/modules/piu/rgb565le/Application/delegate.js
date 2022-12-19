/*---
description: 
flags: [onlyStrict]
---*/

const colorSkin = new Skin({ fill: ["red", "yellow"] });

class StateChangeBehavior {
	changeState(content, state) {
		content.state = state;
	}	
}

new Application(null, {
	skin: colorSkin, Behavior: StateChangeBehavior
});

screen.checkImage("73328e0ea82021eb0cef0ac14a7832ee");

application.delegate("changeState", 1);
screen.checkImage("64668108040303d923bbf1399e359203");

application.delegate("changeState", 0.75);
screen.checkImage("bddb9fbabfdacc319ad3a40ffc2d0f50");