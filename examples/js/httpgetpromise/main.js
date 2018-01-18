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

import {Request} from "http";
import Net from "net";
import Timer from "timer";

function wait(ms)
{
	return new Promise(resolve => Timer.set(resolve, ms));
}

function resolve(domain)
{
	return new Promise((resolve, reject) => {
		Net.resolve(domain, (name, address) => {
			if (address)
				resolve(address);
			else
				reject();
		});
	});
}

function fetch(host, path = "/")
{
	return new Promise((resolve, reject) => {
		let request = new Request({host, path, response: String});
		request.callback = function(message, value) {
			if (5 === message)
				resolve(value);
			else if (message < 0)
				reject(-1);
		}
	});
}

//@@ wifi scan here...

resolve("moddable.tech")
	.then(address => trace(`resolved to ${address}\n`))
	.catch(() => trace("cannot resolve\n"));

wait(3000).then(() => {
	fetch("www.example.com")
		.then(body => trace(body, "\n"))
		.catch(error => trace("failed\n"));
})
