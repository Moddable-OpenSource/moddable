/*---
description: 
flags: [onlyStrict]
---*/

const one = new Content(null, { left: 0, top: 0, height: 50, width: 50, skin: new Skin({ fill: "red"}) });
const two = new Content(null, { left: 0, top: 0, height: 50, width: 50, skin: new Skin({ fill: "yellow"}) });
const three = new Content(null, { left: 0, top: 0, height: 50, width: 50, skin: new Skin({ fill: "blue"}) });

const container = new Row(null, { 
	contents: [three]
});

new Application(null, {
	skin: new Skin({ fill: "white" }),
	contents: [ container ]
});

screen.checkImage("efadef3a732f4534bcd45e3c3bfeffad");

container.insert(one, three);
screen.checkImage("782bfaf53e2e16f609287358083a3e92");

container.insert(two, three);
screen.checkImage("5c8b20d4e2f6e29c14a780e0b1c3088a");