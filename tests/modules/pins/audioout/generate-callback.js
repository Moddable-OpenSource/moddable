/*---
description: 
flags: [module,async]
---*/

import {Mixer} from "pins/audioout"
import Timer from "timer"

let mixer = new Mixer({streams: 1, sampleRate: 12000, numChannels: 1});

let counter = 1;
mixer.enqueue(0, Mixer.Callback, counter++);
mixer.enqueue(0, Mixer.Silence, 100);
mixer.enqueue(0, Mixer.Callback, counter++);
mixer.enqueue(0, Mixer.Silence, 100);

let expected = 1;
mixer.callback = function(value) {
	if (value !== expected)
		$DONE("unexpected callback");

	expected += 1;
	if (10 === expected)
		$DONE();
}

Timer.repeat(() => {
	if (counter > 10)
		return

	mixer.mix(100);

	mixer.enqueue(0, Mixer.Callback, counter++);
	mixer.enqueue(0, Mixer.Silence, 100);
}, 5);
