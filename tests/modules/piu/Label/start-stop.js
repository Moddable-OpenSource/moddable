/*---
description: 
flags: [onlyStrict, async, module]
---*/

import Timer from "timer";

let count = 0;

class SampleBehavior extends Behavior {
	onTimeChanged(content) {
		count += 1;
		content.stop();
	}
}

const content = new Label(null, { Behavior: SampleBehavior });

new Application(null, {
	contents: [content]
});

content.start();

Timer.set(() => {
	if (count == 1) {
		$DONE();
	} else if (count < 1) {
		$DONE("`start` didn't start content's timer");
	} else if (count > 1) {
		$DONE("`stop` didn't stop content's timer");
	}
}, 100);
