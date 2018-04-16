/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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

import Poco from "commodetto/Poco";
import Resource from "Resource";
import Timer from "timer";

let font = new Resource("8x8font.dat");

let render = new Poco(screen);
let black = render.makeColor(0, 0, 0);

const colors = [
	render.makeColor(255, 255, 255),
	render.makeColor(0, 255, 0),
	render.makeColor(255, 0, 0),
	render.makeColor(0, 0, 255),
	render.makeColor(255, 255, 0),
	render.makeColor(0, 255, 255),
	render.makeColor(255, 0, 255),
];

const text = [
	'I heartily accept the motto,',
	'"That government is best which',
	'governs least"; and I should',
	'like to see it acted up to',
	'more rapidly and',
	'systematically. Carried out,',
	'it finally amounts to this,',
	'which also I believe- "That',
	'government is best which',
	'governs not at all"; and when',
	'men are prepared for it, that',
	'will be the kind of government',
	'which they will have.',
	'Government is at best but an',
	'expedient; but most',
	'governments are usually, and',
	'all governments are sometimes,',
	'inexpedient. The objections',
	'which have been brought',
	'against a standing army, and',
	'they are many and weighty, and',
	'deserve to prevail, may also',
	'at last be brought against a',
	'standing government. The',
	'standing army is only an arm',
	'of the standing government.',
	'The government itself, which',
	'is only the mode which the',
	'people have chosen to execute',
	'their will, is equally liable',
	'to be abused and perverted',
	'before the people can act',
	'through it. Witness the',
	'present Mexican war, the work',
	'of comparatively a few',
	// individuals using the standing government as their tool; for, in the outset, the people would not have consented to this measure.
];

render.begin();
	render.fillRectangle(black, 0, 0, render.width, render.height);
render.end();

let color = 0;

Timer.repeat(() => {
	for (let i = 0, y = 0; i < text.length; i++, y += 9) {
		render.begin(0, y, render.width, 8);
			render.fillRectangle(black, 0, 0, render.width, render.height);
			render.drawText(text[i], font, colors[color % colors.length], 0, y, render.width);
		render.end();
	}
	color += 1;
}, 1000 / 12);
