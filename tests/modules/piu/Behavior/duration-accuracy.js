/*---
description: 
flags: [onlyStrict, async]
---*/

const expected = 100;

class TestBehavior extends $TESTMC.Behavior {
	onDisplaying(application) {
		application.duration = expected;
		this.start = Date.now();
		application.start();
	}
	onFinished(application) {
		let delta = (Date.now() - this.start) - expected;
		assert((delta >= -1) && (delta < 3), `Content clock duration of ${expected} off by ${delta}`);
		$DONE();
	}
}

new Application(null, {
	Behavior: TestBehavior
});

$TESTMC.timeout(150, "`onFinished` should have been triggered");