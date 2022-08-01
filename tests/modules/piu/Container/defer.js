/*---
description: 
flags: [onlyStrict, async]
---*/

let events = [];

class SampleBehavior extends $TESTMC.Behavior {
	onTestEvent(content) {
		events.push(1);
		assert.sameValue(events.length, 2, `Incorrect number of events triggered`);
		assert.sameValue(events[0], 0, `Incorrect order of events`);
		assert.sameValue(events[1], 1, `Incorrect order of events`);
		$DONE();
	}	
}

const sampleContent = new Container(null, {
	Behavior: SampleBehavior
})

new Application(null, {
	contents: [ sampleContent ],
});

sampleContent.defer("onTestEvent");
events.push(0);

$TESTMC.timeout(50, "`onTestEvent` should have been triggered");