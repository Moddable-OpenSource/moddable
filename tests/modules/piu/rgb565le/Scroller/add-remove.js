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

const container = new Scroller(null, { 
	left: 0, right: 0, top: 0, bottom: 0,
	contents: [ redSquare, yellowSquare, blueSquare ]
});

new Application(null, {
	skin: new Skin({ fill: "white" }),
	contents: [ container ]
});

screen.checkImage("956d38536582eaab00d12d41ddb05629");

container.remove(yellowSquare);
screen.checkImage("98868b4660b55533461626a949f47956");

container.remove(blueSquare);
screen.checkImage("b1b84e6a1e6ecdcee8643ff21e21963c");

container.remove(redSquare);
screen.checkImage("3b06e6e8da2bfca351d5210df4a736df");

container.add(yellowSquare);
screen.checkImage("02466534568cdcce1d6ccece804eac31");

container.add(blueSquare);
screen.checkImage("98868b4660b55533461626a949f47956");

container.add(redSquare);
screen.checkImage("b1b84e6a1e6ecdcee8643ff21e21963c");