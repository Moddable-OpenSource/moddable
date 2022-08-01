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

const content = new Row(null, {
	top: 0, left: 0, height: 80, width: 80, 
	skin: colorSkin, Behavior: StateChangeBehavior
});

new Application(null, {
	skin: new Skin({ fill: "white" }),
	contents: [ content ]
});

screen.checkImage("968025f9e885cc809dfaa88ad8b354ed");

content.delegate("changeState", 1);
screen.checkImage("4534db1fcf186d0bb681b98b1e40d1c3");

content.delegate("changeState", 0.75);
screen.checkImage("450f83a0baec4783c3eaa60834eda5e3");