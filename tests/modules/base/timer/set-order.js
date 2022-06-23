/*---
description: 
flags: [async, module]
---*/

import Timer from "timer";

const goal = [0, 3, 6, 9, 12, 15, 18, 21];
const forward = [];
const reverse = [];

for (let i = 0; i < goal.length; i++)
	Timer.set(() => forward.push(goal[i]), goal[i]);

for (let i = goal.length - 1; i >= 0; i--)
	Timer.set(() => reverse.push(goal[i]), goal[i]);

Timer.set($DO(() => {
	assert.sameValue(forward.length, goal.length);
	assert.sameValue(reverse.length, goal.length);

	for (let i =  0; i < goal.length; i++) {
		assert.sameValue(forward[i], goal[i]);
		assert.sameValue(reverse[i], goal[i]);
	}
}), 20);
