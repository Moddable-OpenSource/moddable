/*
 * NEEDS BOILERPLATE
 *     Copyright (C) 2016-2017 Moddable Tech, Inc.
 *     All rights reserved.
 */


export default class Radio @ "xs_Radio_destructor" {
	constructor(dictionary) @ "xs_Radio";
	postMessage(msg) @ "xs_Radio_postMessage";
	receiveMessage(msg) {
		this.context.onMessage(msg);
	};
	listen(mode) @ "xs_Radio_listen";
	setTxPower(mode) @ "xs_Radio_setTxPower";
	getTxPower() @ "xs_Radio_getTxPower";
	waitUntilIdle() @ "xs_Radio_waitUntilIdle";
	getUnique() @ "xs_Radio_getUnique";
};

Object.freeze(Radio.prototype);

