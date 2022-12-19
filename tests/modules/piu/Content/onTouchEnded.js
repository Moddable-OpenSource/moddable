/*---
description: 
flags: [async, module]
---*/

import Time from "time";

let expected;

const touching = [
	{x: 0, y: 0},
	{x: 25, y: 25},
	{x: 49, y: 0},
	{x: 0, y: 49}
];

const notTouching = [
	{x: 50, y: 0},
	{x: 0, y: 50},
	{x: 100, y: 100}
];

class SampleBehavior extends $TESTMC.Behavior {	
	onTouchEnded(content, id, x, y, ticks) {
		if (!expected)
			$DONE("unexpected onTouchEnded");

		assert.sameValue(x, expected.x);
		assert.sameValue(y, expected.y);
		expected = "called";
	}
}

const sampleContent = new Content(null, {
	top: 0, left: 0, height: 50, width: 50,
	active: true, Behavior: SampleBehavior
})

new Application(null, {
	contents: [ sampleContent ]
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
		assert.sameValue("called", expected, "onTouchEnded not called");
	}
}).then($DONE, $DONE);