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

const CYCLE_AUTOMATICALLY = 10000;		// ms to cycle to the next effect

const LEN = 144;

const Timing_WS2812B = {
    mark:  { level0: 1, duration0: 900,  level1: 0, duration1: 350, },
    space: { level0: 1, duration0: 350,   level1: 0, duration1: 1000, },
    reset: { level0: 0, duration0: 30000, level1: 0, duration1: 30000, } };

const strand = new NeoStrand({length: LEN, pin: 23, order: "RGB", timing: Timing_WS2812B});

strand.brightness = 128;

let mode = 0;
let schemes = [];

let baseDictionary = { strand };

let marqueeDictionary = Object.assign({}, baseDictionary, { });
let marquee1 = Object.assign({}, baseDictionary,
		{ sizeA: 1, rgbA: { r: 0x80, g: 0, b: 0 },
		  sizeB: 4, rgbB: { r: 0, g: 0, b: 0x80 }
		});
let marquee2 = Object.assign({}, marquee1, { reverse: 1 });
let hueSpanDictionary = Object.assign({}, baseDictionary, { });
let hueSpan1 = Object.assign({}, baseDictionary, { reverse: 1 });
let hueSpan2 = Object.assign({}, baseDictionary, { size: (strand.length / 4) });
let hueSpan3 = Object.assign({}, hueSpan2, { reverse: 1 });
let sineDictionary =  Object.assign({}, baseDictionary, { offset: .5 });
let pulseDictionary = Object.assign({}, baseDictionary, { });
let pulse1 = Object.assign({}, baseDictionary,
		{ direction: 0,
		  duration: 5000,
		  position: "random" });
let patternDictionary = Object.assign({}, baseDictionary, {  });

let pattern1 = Object.assign({}, baseDictionary,
        { pattern: [ 0x003300, 0x003300, 0x003300, 0x131313, 0x131313 ] });

let pattern2 = Object.assign({}, baseDictionary,
		{ pattern: [ 0x110000, 0x008800, 0x008800, 0x008800, 0x008800 ] });

schemes.push( [ new NeoStrand.Marquee( marqueeDictionary ) ]);
schemes.push( [ new NeoStrand.Marquee( marquee1 ) ]);
schemes.push( [ new NeoStrand.Marquee( marquee2 ) ]);
schemes.push( [ new NeoStrand.HueSpan( hueSpanDictionary ) ]);
schemes.push( [ new NeoStrand.HueSpan( hueSpan1 ) ]);
schemes.push( [ new NeoStrand.HueSpan( hueSpan2 ) ]);
schemes.push( [ new NeoStrand.HueSpan( hueSpan3 ) ]);
schemes.push( [ new NeoStrand.Sine( sineDictionary ) ]);
schemes.push( [ new NeoStrand.Pulse( pulseDictionary ) ]);
schemes.push( [ new NeoStrand.Pulse( pulse1 ) ]);
schemes.push( [ new NeoStrand.Pulse( pulse1 ) ,
			    new NeoStrand.Pulse( pulse1 ),
			    new NeoStrand.Pulse( pulse1 ),
			 ]);
schemes.push( [ new NeoStrand.Pattern( patternDictionary ) ]);
schemes.push( [ new NeoStrand.Pattern( pattern1 ) ]);
schemes.push( [ new NeoStrand.Pattern( pattern2) ]);

strand.setScheme(schemes[0]);

strand.start(50);

let monitor = new Monitor({
		pin: 0,
		mode: Digital.InputPullUp,
		edge: Monitor.Falling });
monitor.lastMS = Time.ticks;

monitor.onChanged = function() {
	let t = Time.ticks;
	if ((t - this.lastMS) > 125) {
		mode = (mode + 1) % schemes.length;
		strand.setScheme(schemes[mode]);
	}
	this.lastMS = t;
}

class RandomColor extends NeoStrandEffect {
    constructor(dictionary) {
        super(dictionary);
        this.name = "RandomColor"
		this.size = dictionary.size ? dictionary.size : 5;
		this.max = dictionary.max ? dictionary.max : 64;
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

schemes.push( [ new RandomColor( { strand, loop: 1, duration: 5000 } ) ]);


