/*---
description: 
flags: [async, module]
---*/

import Time from "time";

let count = 0;

const touching = [
	{x: 0, y: 0},
	{x: 25, y: 25},
	{x: 49, y: 0},
	{x: 0, y: 49}
];

class SampleBehavior extends $TESTMC.Behavior {	
	onTouchMoved(content, id, x, y, ticks) {
		count++;
	}
}

const sampleContent = new Column(null, {
	top: 0, left: 0, height: 50, width: 50,
	active: true, Behavior: SampleBehavior
})

new Application(null, {
	contents: [ sampleContent ],
});

screen.doTouchBegan(0, 1, 1, Time.ticks); 
for (let coordinate of touching)
	await screen.doTouchMoved(0, coordinate.x, coordinate.y, Time.ticks);

screen.doTouchEnded(0, 25, 25, Time.ticks)
.then(() => assert.sameValue(count, 1 + touching.length, `onTouchMoved should have been triggered ${touching.length} times but was triggered ${count} times`))
.then($DONE, $DONE); 