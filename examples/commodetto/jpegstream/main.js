/*
 * Copyright (c) 2018-2020  Moddable Tech, Inc.
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

import {Request} from "http"
import JPEG from "commodetto/readJPEG";
import Poco from "commodetto/Poco";

class JPEGRequest extends Request {
	constructor(dictionary) {
		super(dictionary);
		this.poco = dictionary.poco;
		this.resolve = dictionary.resolve;
		this.reject = dictionary.reject;
	}
	callback(message, value, etc) {
		if (message < 0) {
			this.reject();
			return;
		}

		if (1 == message)
			this.jpeg = new JPEG();
		else if (4 == message)
			this.jpeg.push(this.read(ArrayBuffer));
		else if (5 == message) {
			this.jpeg.push();
			this.resolve();
		}

		while (this.jpeg.ready) {
			let block = this.jpeg.read();
			this.poco.begin(block.x, block.y, block.width, block.height);
			this.poco.drawBitmap(block, block.x, block.y);
			this.poco.end();
		}
	}
}

function display(poco, host, path = "/")
{
	return new Promise((resolve, reject) => {
	   new JPEGRequest({host, path, poco, resolve, reject});
	});
}


const paths = [
	"/images/patrick-team.jpg",
	"/images/peter-team.jpg",
	"/images/brian-team.jpg",
	"/images/lizzie-team.jpg",
	"/images/andy-team.jpg",
	"/images/mike-team.jpg",
	"/images/chris-team.jpg",
];

async function main() {
	let poco = new Poco(screen);
	poco.begin();
	poco.fillRectangle(poco.makeColor(0, 0, 0), 0, 0, poco.width, poco.height);
	poco.end();

	let index = 0;
	while (true) {
		try {
			await display(poco, "test.moddable.com", paths[index]);
		}
		catch {
		}
		index = (index + 1) % paths.length;
	}
}

export default main;
