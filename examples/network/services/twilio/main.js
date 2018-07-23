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

import {Request} from "http"
import SecureSocket from "securesocket";
import Base64 from "base64";

const account = "AC3ed2ef12baac0efd571d334ea40ba684";
const token = "76e437f2332a97f2e72ddecd26dcd430";

let params = {
	To: "+14152794077",
	From: "+16507795272",
	Body: "hullo, from moddable.",
};

let body = "";
for (let key in params) {
	if (body)
		body += "&";
	body += key + "=" + encodeURIComponent(params[key]);
}

let request = new Request({
	host: "api.twilio.com",
	path: `/2010-04-01/Accounts/${account}/Messages.json`,
	method: "POST",
	headers: [	"content-type", "application/x-www-form-urlencoded",
				"content-length", body.length,
				"Authorization", "Basic " + Base64.encode(account + ":" + token)],
	body: body,
	response: String,
	port: 443,
	Socket: SecureSocket
});

request.callback = function(message, value)
{
	if (5 == message) {
		trace("SMS sent\n");
		trace(value, "\n");
	}
	else if (message < 0)
		trace("SMS send HTTP request failed\n");
}
