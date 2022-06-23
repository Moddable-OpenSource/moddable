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

const col = new Column(null, {
	top: 0, bottom: 0, left: 0, right: 0,
	skin: new Skin({ fill: "white" }),
	contents: [redSquare, yellowSquare]
});

new Application(null, { contents: [col] });
screen.checkImage("3ac52d95d4dd044f3ccf4a73d69dd7e7");

col.replace(redSquare, blueSquare);
screen.checkImage("41016096e2c4133d7cbc06adac02b0dc");

col.replace(yellowSquare, redSquare);
screen.checkImage("055298d4930fd87709eba878f82c55ed");

col.replace(blueSquare, yellowSquare);
screen.checkImage("b6ee7eb85bd72d02de6de645f3794efe");
