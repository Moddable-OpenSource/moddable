/*---
description: 
flags: [onlyStrict]
---*/

const ColoredSquare = Content.template($ => ({
	height: 0, width: 0,
	skin: new Skin({ fill: $ }),
}));

class LayoutBehavior extends $TESTMC.Behavior {
	onMeasureHorizontally(layout, width) {
		let squareWidth = width/2;
		let square = layout.first;
		while (square) {
			square.sizeBy(squareWidth-square.width, 0);
			square = square.next;
		}
		return width;
	}
	onMeasureVertically(layout, height) {
		let squareHeight = height/2;
		let square = layout.first;
		while (square) {
			square.sizeBy(0, squareHeight-square.height);
			square = square.next;
		}
		return height;
	}
}

const sampleLayout = new Layout(null, {
	width: 240, height: 320,
	skin: new Skin({fill: "black"}),
	contents: [
		new ColoredSquare("red", {top: 0, left: 0}),
		new ColoredSquare("yellow", {top: 0, right: 0}),
		new ColoredSquare("green", {bottom: 0, left: 0}),
		new ColoredSquare("blue", {bottom: 0, right: 0}),
	],
	Behavior: LayoutBehavior
});

new Application(null, { 
	skin: new Skin({fill: "white"}),
	contents: [sampleLayout] 
});
screen.checkImage("55d250454a69e2c8fb5113bc1370321e");

sampleLayout.width = 100;
screen.checkImage("c8ef68038073baa72bcd86df7693c56b");

sampleLayout.height = 100;
screen.checkImage("44f1a7e2a2a058657baa785b277a1687");