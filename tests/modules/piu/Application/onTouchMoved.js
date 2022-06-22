/*---
description: 
flags: [async, module]
---*/

import Time from "time";

let count = 0;

const touching = [
	{x: 0, y: 0},
	{x: 0, y: 319},
	{x: 239, y: 0},
	{x: 0, y: 319}
];

class SampleBehavior extends $TESTMC.Behavior {	
	onTouchMoved(content, id, x, y, ticks) {
		count++;
	}
}

new Application(null, {
	active: true, Behavior: SampleBehavior
});

screen.doTouchBegan(0, 1, 1, Time.ticks); 
for (let coordinate of touching)
	await screen.doTouchMoved(0, coordinate.x, coordinate.y, Time.ticks);

screen.doTouchEnded(0, 25, 25, Time.ticks)
.then(() => assert.sameValue(count, 1 + touching.length, `onTouchMoved should have been triggered ${touching.length} times but was triggered ${count} times`))
.then($DONE, $DONE); 