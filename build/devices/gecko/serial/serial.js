/*
 * NEEDS BOILERPLATE
 *     Copyright (C) 2016-2017 Moddable Tech, Inc.
 *     All rights reserved.
 */

export default class Serial @ "xs_Serial_destructor" {
	constructor(dictionary) @ "xs_Serial";
	send() @ "xs_Serial_send";
	receive() @ "xs_Serial_receive";
};

Object.freeze(Serial.prototype);

