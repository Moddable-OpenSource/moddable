/*---
description: 
flags: [onlyStrict, async]
---*/

const expected = 30;
let count = 5;

class TestBehavior extends $TESTMC.Behavior {
	onDisplaying(application) {
		application.interval = expected;
		application.start();
	}
	onTimeChanged(application) {
		if (!this.start) {
			// `onTimeChanged` triggers shortly after calling `start` (not `interval` ms after), so don't check delta here
			this.start = Date.now();
		} else {
			let now = Date.now();
			let delta = (now - this.start) - expected;
			this.start = now;
			assert((delta >= -1) && (delta < 3), `Content clock duration of ${expected} off by ${delta}`);
			count--;
			if (!count) {
				application.stop();
				$DONE();
			}
		}
	}
}

new Application(null, {
	Behavior: TestBehavior
});

$TESTMC.timeout(250, "`onFinished` should have been triggered");