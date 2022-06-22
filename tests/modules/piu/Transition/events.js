/*---
description: 
flags: [onlyStrict, async]
---*/

let events = [];

class SampleTransition extends Transition {
	constructor() {
		super(50);
	}
	onBegin(container) {
		assert(container.transitioning,"Container's `transitioning` property should be `true`");
		events.push(0);
	}
	onStep(fraction) {
		events.push(fraction);
	}
	onEnd(container) {
		assert(!container.transitioning,"Container's `transitioning` property should be `false`");
		events.push(2);
		let prev = -1;
		let index = 0;
		try {
			while (index < events.length) {
				let curr = events[index];
				assert((curr >= prev),`Incorrect order of events (${curr} came after ${prev})`);
				prev = curr;
				index++;
			}
		} catch(e) {
			return $DONE(e);
		}
		$DONE();
	}
}

let transition = new SampleTransition();

let container1 = new Container;
let container2 = new Container;

new Application(null, {
	contents: [container1]
});

application.run(transition, container1, container2);

$TESTMC.timeout(100, "`onEnd` should have been triggered");