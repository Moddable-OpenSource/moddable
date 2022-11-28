/*---
description: 
flags: [module,async]
---*/

import AudioOut from "pins/audioout"
import Resource from "Resource";
import Timer from "timer";

const maud = new Resource("bflatmajor-ima-12000.maud");

const out = new AudioOut({streams: 2, sampleRate: 12000, numChannels: 1});

out.enqueue(0, AudioOut.Samples, maud, Infinity);
out.start();

let stage = 0;
Timer.repeat(() => {
	switch (stage++) {
		case 0:	out.stop(); break;
		case 1:
			out.enqueue(1, AudioOut.Tone, 330, Infinity);
			out.start();
			break;
		case 2:
			out.enqueue(1, AudioOut.Flush);
			out.stop();
			break;
		case 3:	out.start(); break;
		case 4:	out.stop(); break;
		case 5:	out.start(); break;
		case 6:	
			out.enqueue(1, AudioOut.Tone, 660, Infinity);
			break;
		case 7:
			out.enqueue(1, AudioOut.Flush);
			break;
		case 8:
			out.enqueue(0, AudioOut.Flush);
			break;
		case 9:
			out.close();
			$DONE();
			break;
	}

}, 500);
