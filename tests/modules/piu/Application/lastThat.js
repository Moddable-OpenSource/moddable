/*---
description: 
flags: [onlyStrict]
---*/

let triggered;

class SampleBehavior extends Behavior {
	sampleEvent(content) {
		triggered = content;
		return true;
	}
}

const one = new Content;
const two = new Content(null, {Behavior: SampleBehavior});
const three = new Content(null, {Behavior: SampleBehavior});

new Application(null, {
	contents: [ one, two, three ],
	Behavior: SampleBehavior
});

application.lastThat("sampleEvent")
assert.sameValue(triggered, three, "sampleEvent should have been triggered by `three`");

application.remove(three);
triggered = undefined;
application.lastThat("sampleEvent")
assert.sameValue(triggered, two, "sampleEvent should have been triggered by `two`");

application.remove(two);
triggered = undefined;
application.lastThat("sampleEvent")
assert.sameValue(triggered, undefined, "sampleEvent should not have been triggered");

triggered = undefined;
application.lastThat("nonexistentEvent")
assert.sameValue(triggered, undefined, "nonexistentEvent should not have been triggered");