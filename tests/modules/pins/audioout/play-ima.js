/*---
description: 
flags: [module,async]
---*/

import AudioOut from "pins/audioout"
import Resource from "Resource";
import Timer from "timer";

const maud = new Resource("bflatmajor-ima-12000.maud");

const out = new AudioOut({streams: 1, sampleRate: 12000, numChannels: 1});

out.enqueue(0, AudioOut.Samples, maud);
out.start();

Timer.set(() => $DONE(), 2000)
