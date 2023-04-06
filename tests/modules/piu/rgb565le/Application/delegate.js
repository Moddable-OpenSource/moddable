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

screen.checkImage("73d09edd8afd47d9038203b6c975f44c");

application.delegate("changeState", 1);
screen.checkImage("ebc4432964a5c602347717a831ce4163");

application.delegate("changeState", 0.75);
screen.checkImage("a9e044860fb4d76fed1e79d5f63ea7c3");