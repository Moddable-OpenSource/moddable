/*
 * Copyright (c) 2021-2022  Moddable Tech, Inc.
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

import { fetch, Headers } from "fetch";
import { URLSearchParams } from "url";

const headers = new Headers([
    ['Content-Type', 'application/x-www-form-urlencoded;charset=UTF-8'],
	["Date", Date()],
	["User-Agent", "ecma-419 test"]
]);

const body = new URLSearchParams([
	["Date", Date()],
	["Input", "This is no input!"]
]);

// HTTP

fetch("http://httpbin.org/post", { method:"POST", headers, body })
.then(response => {
	trace(`\n${response.url} ${response.status} ${response.statusText}\n\n`);
	response.headers.forEach((value, key) => trace(`${key}: ${value}\n`));
	trace("\n");
	return response.json();
})
.then(json => {
	trace(JSON.stringify(json, null, "\t"));
	trace("\n");
});

fetch("http://httpbin.org/put", { method:"PUT", body:"This is no data!" })
.then(response => {
	trace(`\n${response.url} ${response.status} ${response.statusText}\n\n`);
	response.headers.forEach((value, key) => trace(`${key}: ${value}\n`));
	trace("\n");
	return response.json();
})
.then(json => {
	trace(JSON.stringify(json, null, "\t"));
	trace("\n");
});

fetch("http://httpbin.org/gzip")
.then(response => {
	trace(`\n${response.url} ${response.status} ${response.statusText}\n\n`);
	response.headers.forEach((value, key) => trace(`${key}: ${value}\n`));
	trace("\n");
	return response.arrayBuffer();
})
.then(arrayBuffer => {
	const array = new Uint8Array(arrayBuffer);
	trace(array + "\n");
});

fetch("http://httpbin.org/json")
.then(response => {
	trace(`\n${response.url} ${response.status} ${response.statusText}\n\n`);
	response.headers.forEach((value, key) => trace(`${key}: ${value}\n`));
	trace("\n");
	return response.json();
})
.then(json => {
	trace(JSON.stringify(json, null, "\t"));
	trace("\n");
});

fetch("http://httpbin.org/encoding/utf8")
.then(response => {
	trace(`\n${response.url} ${response.status} ${response.statusText}\n\n`);
	response.headers.forEach((value, key) => trace(`${key}: ${value}\n`));
	trace("\n");
	return response.text();
})
.then(text => {
	let c  = text.length;
	for (let i = 0; i < c; i++)
		trace(text[i]);
	trace("\n");
});

// HTTPS

fetch("https://httpbin.org/post", { method:"POST", headers, body })
.then(response => {
	trace(`\n${response.url} ${response.status} ${response.statusText}\n\n`);
	response.headers.forEach((value, key) => trace(`${key}: ${value}\n`));
	trace("\n");
	return response.json();
})
.then(json => {
	trace(JSON.stringify(json, null, "\t"));
	trace("\n");
});

fetch("https://httpbin.org/put", { method:"PUT", body:"This is no data!" })
.then(response => {
	trace(`\n${response.url} ${response.status} ${response.statusText}\n\n`);
	response.headers.forEach((value, key) => trace(`${key}: ${value}\n`));
	trace("\n");
	return response.json();
})
.then(json => {
	trace(JSON.stringify(json, null, "\t"));
	trace("\n");
});

fetch("https://httpbin.org/gzip")
.then(response => {
	trace(`\n${response.url} ${response.status} ${response.statusText}\n\n`);
	response.headers.forEach((value, key) => trace(`${key}: ${value}\n`));
	trace("\n");
	return response.arrayBuffer();
})
.then(arrayBuffer => {
	const array = new Uint8Array(arrayBuffer);
	trace(array + "\n");
});

fetch("https://httpbin.org/json")
.then(response => {
	trace(`\n${response.url} ${response.status} ${response.statusText}\n\n`);
	response.headers.forEach((value, key) => trace(`${key}: ${value}\n`));
	trace("\n");
	return response.json();
})
.then(json => {
	trace(JSON.stringify(json, null, "\t"));
	trace("\n");
});

fetch("https://httpbin.org/encoding/utf8")
.then(response => {
	trace(`\n${response.url} ${response.status} ${response.statusText}\n\n`);
	response.headers.forEach((value, key) => trace(`${key}: ${value}\n`));
	trace("\n");
	return response.text();
})
.then(text => {
	let c  = text.length;
	for (let i = 0; i < c; i++)
		trace(text[i]);
	trace("\n");
});

