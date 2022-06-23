/*---
description: 
flags: [onlyStrict]
---*/

const Square = Content.template($ => ({
	top: 10, left: 10, width: 50, height: 50,
	skin: new Skin({ fill: $ })
}));

const redSquare = new Square("red");
const yellowSquare = new Square("yellow");
const blueSquare = new Square("blue");

const row = new Row(null, {
	top: 0, bottom: 0, left: 0, right: 0,
	skin: new Skin({ fill: "white" }),
	contents: [ redSquare, yellowSquare ]
});

new Application(null, { contents: [row] });

screen.checkImage("3391308cef1310e5bf4c199b14ac331f");

row.replace(redSquare, blueSquare);
screen.checkImage("e064ac84f65e8a8a589d1fa8c9e386cb");

row.replace(yellowSquare, redSquare);
screen.checkImage("5de359101fd8482cbdaff13ef943a590");

row.replace(blueSquare, yellowSquare);
screen.checkImage("70cb414b9f919d87ecba0a02001cf076");