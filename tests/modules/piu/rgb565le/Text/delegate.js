/*---
description: 
flags: [onlyStrict]
---*/

const style = new Style({ font:"16px Open Sans", color: ["black", "white"] });
const colorSkin = new Skin({  fill: ["red", "yellow"] });

class StateChangeBehavior {
	changeState(content, state) {
		content.state = state;
	}	
}

const content = new Text(null, {
	height: 50, width: 50, style,
	skin: colorSkin, string: "test", 
	Behavior: StateChangeBehavior
});

new Application(null, {
	skin: new Skin({ fill: "white" }),
	contents: [ content ]
});

screen.checkImage("cbca45a370d984e48cf86de813d286b9");

content.delegate("changeState", 1);
screen.checkImage("39852254fb15186d5e1f34e2b40aa517");

content.delegate("changeState", 0.75);
screen.checkImage("bd38fe95d6fd806eedbad48eee088e9b");