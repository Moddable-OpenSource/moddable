/*---
description: 
flags: [onlyStrict, async]
---*/

let events = [];

class SampleBehavior extends $TESTMC.Behavior {
	onTransitionBeginning(container) {
		events.push(0);
	}
	onTransitionEnded(container) {
		events.push(1);
		assert.sameValue(events.length, 2, `Incorrect number of events triggered`);
		assert.sameValue(events[0], 0, `Incorrect order of events`);
		assert.sameValue(events[1], 1, `Incorrect order of events`);
		$DONE();
	}
}

class SampleTransition extends Transition {
	constructor() {
		super(5);
	}
}

const transition = new SampleTransition();
new Application(null, { Behavior: SampleBehavior });
application.run(transition);

$TESTMC.timeout(30, "`onTransitionEnded` should have been triggered");