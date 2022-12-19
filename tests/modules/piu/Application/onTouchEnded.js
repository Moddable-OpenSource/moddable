/*---
description: 
flags: [async, module]
---*/

import Time from "time";

let expected;

const touching = [
	{x: 0, y: 0},
	{x: 0, y: 319},
	{x: 239, y: 0},
	{x: 0, y: 319}
];

const notTouching = [
	{x: 240, y: 0},
	{x: 0, y: 320},
	{x: 240, y: 320}
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

new Application(null, {
	active: true, Behavior: SampleBehavior
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