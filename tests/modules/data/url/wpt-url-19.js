/*---
description: https://github.com/web-platform-tests/wpt/url/resources/urltestdata.json
flags: [module]
---*/

import { runTests } from "./url_FIXTURE.js";

const tests = [
	{
		"input": "http://%3g%78%63%30%2e%30%32%35%30%2E.01",
		"base": "http://other.com/",
		"failure": true
	},
	"A space in a host causes failure",
	{
		"input": "http://192.168.0.1 hello",
		"base": "http://other.com/",
		"failure": true
	},
	{
		"input": "https://x x:12",
		"base": "about:blank",
		"failure": true
	},
	"Domains with empty labels",
	{
		"input": "http://./",
		"base": "about:blank",
		"href": "http://./",
		"origin": "http://.",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": ".",
		"hostname": ".",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	{
		"input": "http://../",
		"base": "about:blank",
		"href": "http://../",
		"origin": "http://..",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "..",
		"hostname": "..",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	"Broken IPv6",
	{
		"input": "http://[www.google.com]/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://[google.com]",
		"base": "http://other.com/",
		"failure": true
	},
	{
		"input": "http://[::1.2.3.4x]",
		"base": "http://other.com/",
		"failure": true
	},
	{
		"input": "http://[::1.2.3.]",
		"base": "http://other.com/",
		"failure": true
	},
	{
		"input": "http://[::1.2.]",
		"base": "http://other.com/",
		"failure": true
	},
	{
		"input": "http://[::1.]",
		"base": "http://other.com/",
		"failure": true
	},
	"Misc Unicode"
];

runTests(tests);
