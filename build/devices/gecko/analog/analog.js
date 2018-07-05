/*
 * NEEDS BOILERPLATE
 *     Copyright (C) 2016-2017 Moddable Tech, Inc.
 *     All rights reserved.
 */


export default class Analog @ "xs_Analog_destructor" {
	constructor(dictionary) {
		global.sleepers.push(Analog.sleepEM4);
		this.construct(dictionary);
	};
	construct(dictionary) @ "xs_Analog";
	static sleepEM4() @ "xs_Analog_sleepEM4";
	read(channel) @ "xs_Analog_read";
};

Object.freeze(Analog.prototype);
