/*
 * Copyright (c) 2018  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 *
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */
import Timer from "timer";
import Time from "time";
import Digital from "pins/digital";
import Monitor from "pins/digital/monitor";
import { NeoStrand, NeoStrandEffect } from "neostrand";

const Timing_WS2812B = {
    mark:  { level0: 1, duration0: 900,  level1: 0, duration1: 350, },
    space: { level0: 1, duration0: 350,   level1: 0, duration1: 900, },
    reset: { level0: 0, duration0: 100, level1: 0, duration1: 100 }};

const LEN = 144;
const strand = new NeoStrand({length: LEN, pin: 22, order: "RGB", timing: Timing_WS2812B});

let mode = 0;

let baseDictionary = { strand };

let myEffect  = new NeoStrand.HueSpan({ strand, start: 0, end: (strand.length/2 - 1) });
let myEffect2 = new NeoStrand.Marquee({ strand, start: (strand.length/2), end: strand.length, reverse: 1 });


let myScheme = [ myEffect, myEffect2 ];

strand.setScheme(myScheme);
strand.start(50);

let manySchemes = [
	myScheme,
	[ new NeoStrand.HueSpan({ strand }) ],
	[ new NeoStrand.Marquee({ strand }) ],
];
let currentScheme = 0;


let monitor = new Monitor({
		pin: 0,
		mode: Digital.InputPullUp,
		edge: Monitor.Falling });

monitor.onChanged = function() { 
    currentScheme = (currentScheme + 1) % manySchemes.length; 
    strand.setScheme(manySchemes[currentScheme]);
}


// for web-based control
import MDNS from "mdns";
import {Server} from "http";

/* claim the name "lights".
	Provide a HTTP server at "http://lights.local"
	Every time a reqest is made, change to the next scheme
 */
let hostName = "lights";

new MDNS({hostName});

let server = new Server({port: 80});
server.callback = function(message, value) {
	if (2 == message)
		this.path = value;

	if (8 == message) {
		if (this.path == "/") {
            // change the scheme
            currentScheme = (currentScheme + 1) % manySchemes.length; 
            strand.setScheme(manySchemes[currentScheme]);

			let body = "Scheme changed to: ";
			for (let i=0; i<strand.scheme.length; i++)
				body += strand.scheme[i].name + "\n";
			trace(body);
			return {headers: ["Content-Type", "text/plain"], body};
		}
		else
			this.status = 404;
	}
}

class RandomColor extends NeoStrandEffect {
	constructor(dictionary) {
		super(dictionary);
		this.name = "RandomColor"
		this.size = dictionary.size ? dictionary.size : 15;
		this.max = dictionary.max ? dictionary.max : 127;
		this.loop = 1;                                      // force loop
	}
	loopPrepare(effect) { 
		effect.colors_set = 0; 
	}
	activate(effect) {
		effect.timeline.on(effect, { effectValue: [ 0, effect.dur ] }, effect.dur, null, 0);
		effect.reset(effect);
	}
	set effectValue(value) {
		if (0 == this.colors_set) {
			for (let i=this.start; i<this.end; i++) {
				if (0 == (i % this.size))
					this.color = this.strand.makeRGB(Math.random() * this.max, Math.random() * this.max, Math.random() * this.max);
				this.strand.set(i, this.color, this.start, this.end);
			}
			this.colors_set = 1;
		}
	}
}

let randomColorScheme = [ new RandomColor({ strand }) ];
manySchemes.push( randomColorScheme );

