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

screen.checkImage("013c41d92b1045c26cfa49500dd2e4f9");

content.delegate("changeState", 1);
screen.checkImage("f84900bc8997f1c647c471cf72d4631f");

content.delegate("changeState", 0.75);
screen.checkImage("66630761b9551953ea90398d6f540fed");