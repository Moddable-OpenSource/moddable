/*---
description: 
flags: [onlyStrict, async]
---*/

class SampleTransition extends Transition {
	constructor() {
		super(5);
	}
	onBegin(container, ...args) {
		try {
			for (let i in args) {
				assert(expected[i] == args[i],`Incorrect arguments passed into onBegan`);
			}
		} catch(e) {
			return $DONE(e);
		}
	}
	onEnd(container, ...args) {
		try {
			for (let i in args) {
				assert(expected[i] == args[i],`Incorrect arguments passed into onEnd`);
			}
			$DONE();
		} catch(e) {
			return $DONE(e);
		}
	}
}

new Application;

const expected = [ "test", 123, {}, [], new Container ]
const transition = new SampleTransition();
application.run(transition, ...expected);

$TESTMC.timeout(50, "`onEnd` should have been triggered");