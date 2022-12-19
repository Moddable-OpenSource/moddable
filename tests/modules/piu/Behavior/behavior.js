/*---
description: 
flags: [onlyStrict, async]
---*/

let dataObject = {};
let events = [];

class TestBehavior extends $TESTMC.Behavior {
	onCreate(application, data) {
		events.push(0);
		assert.sameValue(data, dataObject, "`dataObject` should be passed into `onCreate` function");
	}
	onDisplaying(application) {
		events.push(1);
		application.duration = 50;
		application.start();
	}
	onTimeChanged(application) {
		events.push(2);
	}
	onFinished(application) {
		events.push(3);
		let prev = -1;
		let index = 0;
		while (index < events.length) {
			let curr = events[index];
			assert(curr >= prev, `Incorrect order of events`);
			prev = curr;
			index++;
		}
		$DONE();
	}
}

new Application(dataObject, {
	Behavior: TestBehavior
});

$TESTMC.timeout(100, "`onFinished` should have been triggered");