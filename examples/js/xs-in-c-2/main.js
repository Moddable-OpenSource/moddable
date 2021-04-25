/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <https://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */


class Test {
	constructor() {
		this.data = [0, 1, 2, 3];
	}
	getIndex(x) @ "Test_prototype_getIndex"
	hasIndex(x) @ "Test_prototype_hasIndex"
	setIndex(x, y) @ "Test_prototype_setIndex"
}

const test = new Test;
trace(`${test.getIndex(1)}\n`);
trace(`${test.hasIndex(2)}\n`);
trace(`${test.setIndex(3, 4)}\n`);
trace(`${test.getIndex(3)}\n`);
