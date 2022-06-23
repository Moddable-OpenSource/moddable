/*---
description: 
flags: [async, module]
---*/

import Time from "time";

let expected;

const touching = [
	{x: 10, y: 10},
	{x: 25, y: 25},
	{x: 44, y: 5},
	{x: 5, y: 44}
];

const notTouching = [
	{x: 49, y: 0},
	{x: 0, y: 49},
	{x: 100, y: 100}
];

class CapturingBehavior extends Behavior {
    onTouchBegan(content, id, x, y, ticks) {
        content.captureTouch(id, x, y, ticks);
    }
}

const capturingContent = new Port(null, {
    top: 5, left: 5, height: 40, width: 40,
    skin: new Skin({ fill: "black"}),
    active: true, Behavior: CapturingBehavior
});

class SampleBehavior extends $TESTMC.Behavior {
	onTouchCancelled(content, id, x, y, ticks) {
		if (!expected)
			$DONE("unexpected onTouchCancelled");

		assert.sameValue(x, expected.x);
		assert.sameValue(y, expected.y);
		expected = "called";
	}	
}

const sampleContainer = new Container(null, {
	top: 0, left: 0, height: 50, width: 50,
	contents: capturingContent,
	active: true, backgroundTouch: true, Behavior: SampleBehavior
});

new Application(null, {
	contents: [ sampleContainer ]
});


Promise.resolve()
.then(async () => {
	for (let coordinate of notTouching) {
		await screen.doTouchBegan(0, coordinate.x, coordinate.y, Time.ticks);
		await screen.doTouchEnded(0, coordinate.x, coordinate.y, Time.ticks);
	}

	for (let coordinate of touching) {
		expected = coordinate; 
		await screen.doTouchBegan(0, coordinate.x, coordinate.y, Time.ticks);
		await screen.doTouchEnded(0, coordinate.x, coordinate.y, Time.ticks);
		assert.sameValue("called", expected, "onTouchCancelled not called");
	}
}).then($DONE, $DONE);