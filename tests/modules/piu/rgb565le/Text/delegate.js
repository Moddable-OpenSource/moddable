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

screen.checkImage("e29c52234f0c20262fac0e3abe321647");

content.delegate("changeState", 1);
screen.checkImage("48d847efb704b28f2f9266d0a70b66dc");

content.delegate("changeState", 0.75);
screen.checkImage("782145007933c364da75071e9715af4a");