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

const content = new Label(null, {
	height: 50, width: 50, style,
	skin: colorSkin, string: "test", 
	Behavior: StateChangeBehavior
});

new Application(null, {
	skin: new Skin({ fill: "white" }),
	contents: [ content ]
});

screen.checkImage("e761a5fe298a1eea529d5c0e55e263fb");

content.delegate("changeState", 1);
screen.checkImage("266b94675e21de568c7a280d501f7be4");

content.delegate("changeState", 0.75);
screen.checkImage("2da0bf1b096bf54cdc2423ec7079435e");