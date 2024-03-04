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
	console.log(`${response.url} ${response.status} ${response.statusText}\n\n`);
	response.headers.forEach((value, key) => console.log(`${key}: ${value}\n`));
	return response.json();
})
.then(json => {
	console.log(JSON.stringify(json, null, "\t"));
});

fetch("http://httpbin.org/put", { method:"PUT", body:"This is no data!" })
.then(response => {
	console.log(`${response.url} ${response.status} ${response.statusText}\n\n`);
	response.headers.forEach((value, key) => console.log(`${key}: ${value}\n`));
	return response.json();
})
.then(json => {
	console.log(JSON.stringify(json, null, "\t"));
});

fetch("http://httpbin.org/gzip")
.then(response => {
	console.log(`${response.url} ${response.status} ${response.statusText}\n\n`);
	response.headers.forEach((value, key) => console.log(`${key}: ${value}\n`));
	return response.arrayBuffer();
})
.then(arrayBuffer => {
	const array = new Uint8Array(arrayBuffer);
	console.log(array + "\n");
});

fetch("http://httpbin.org/json")
.then(response => {
	console.log(`${response.url} ${response.status} ${response.statusText}\n\n`);
	response.headers.forEach((value, key) => console.log(`${key}: ${value}\n`));
	return response.json();
})
.then(json => {
	console.log(JSON.stringify(json, null, "\t"));
});

fetch("http://httpbin.org/encoding/utf8")
.then(response => {
	console.log(`${response.url} ${response.status} ${response.statusText}\n\n`);
	response.headers.forEach((value, key) => console.log(`${key}: ${value}\n`));
	return response.text();
})
.then(text => {
	const c  = text.length;
	for (let i = 0; i < c; i++) {
		let lf = text.indexOf("\n", i);
		if (-1 === lf) lf = c;
		console.log(text.substring(i, lf));
		i = lf + 1;
	}
});

// HTTPS

 fetch("https://httpbin.org/post", { method:"POST", headers, body })
 .then(response => {
 	console.log(`${response.url} ${response.status} ${response.statusText}\n\n`);
 	response.headers.forEach((value, key) => console.log(`${key}: ${value}\n`));
 	return response.json();
 })
 .then(json => {
 	console.log(JSON.stringify(json, null, "\t"));
 });
 
 fetch("https://httpbin.org/put", { method:"PUT", body:"This is no data!" })
 .then(response => {
 	console.log(`${response.url} ${response.status} ${response.statusText}\n\n`);
 	response.headers.forEach((value, key) => console.log(`${key}: ${value}\n`));
 	return response.json();
 })
 .then(json => {
 	console.log(JSON.stringify(json, null, "\t"));
 });
 
 fetch("https://httpbin.org/gzip")
 .then(response => {
 	console.log(`${response.url} ${response.status} ${response.statusText}\n\n`);
 	response.headers.forEach((value, key) => console.log(`${key}: ${value}\n`));
 	return response.arrayBuffer();
 })
 .then(arrayBuffer => {
 	const array = new Uint8Array(arrayBuffer);
 	console.log(array + "\n");
 });
 
 fetch("https://httpbin.org/json")
 .then(response => {
 	console.log(`${response.url} ${response.status} ${response.statusText}\n\n`);
 	response.headers.forEach((value, key) => console.log(`${key}: ${value}\n`));
 	return response.json();
 })
 .then(json => {
 	console.log(JSON.stringify(json, null, "\t"));
 });
 
 fetch("https://httpbin.org/encoding/utf8")
 .then(response => {
 	console.log(`${response.url} ${response.status} ${response.statusText}\n\n`);
 	response.headers.forEach((value, key) => console.log(`${key}: ${value}\n`));
 	return response.text();
 })
 .then(text => {
	const c  = text.length;
	for (let i = 0; i < c; i++) {
		let lf = text.indexOf("\n", i);
		if (-1 === lf) lf = c;
		console.log(text.substring(i, lf));
		i = lf + 1;
	}
 });
 
