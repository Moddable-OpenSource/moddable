/*---
description: https://github.com/web-platform-tests/wpt/url/resources/urltestdata.json
flags: [module]
---*/

import { runTests } from "./url_FIXTURE.js";

const tests = [
	"Port overflow (2^64 + 81)",
	{
		"input": "http://f:18446744073709551697/c",
		"base": "http://example.org/",
		"failure": true
	},
	"Port overflow (2^128 + 81)",
	{
		"input": "http://f:340282366920938463463374607431768211537/c",
		"base": "http://example.org/",
		"failure": true
	},
	"# Non-special-URL path tests",
	{
		"input": "sc://ñ",
		"base": "about:blank",
		"href": "sc://%C3%B1",
		"origin": "null",
		"protocol": "sc:",
		"username": "",
		"password": "",
		"host": "%C3%B1",
		"hostname": "%C3%B1",
		"port": "",
		"pathname": "",
		"search": "",
		"hash": ""
	},
	{
		"input": "sc://ñ?x",
		"base": "about:blank",
		"href": "sc://%C3%B1?x",
		"origin": "null",
		"protocol": "sc:",
		"username": "",
		"password": "",
		"host": "%C3%B1",
		"hostname": "%C3%B1",
		"port": "",
		"pathname": "",
		"search": "?x",
		"hash": ""
	},
	{
		"input": "sc://ñ#x",
		"base": "about:blank",
		"href": "sc://%C3%B1#x",
		"origin": "null",
		"protocol": "sc:",
		"username": "",
		"password": "",
		"host": "%C3%B1",
		"hostname": "%C3%B1",
		"port": "",
		"pathname": "",
		"search": "",
		"hash": "#x"
	},
	{
		"input": "#x",
		"base": "sc://ñ",
		"href": "sc://%C3%B1#x",
		"origin": "null",
		"protocol": "sc:",
		"username": "",
		"password": "",
		"host": "%C3%B1",
		"hostname": "%C3%B1",
		"port": "",
		"pathname": "",
		"search": "",
		"hash": "#x"
	},
	{
		"input": "?x",
		"base": "sc://ñ",
		"href": "sc://%C3%B1?x",
		"origin": "null",
		"protocol": "sc:",
		"username": "",
		"password": "",
		"host": "%C3%B1",
		"hostname": "%C3%B1",
		"port": "",
		"pathname": "",
		"search": "?x",
		"hash": ""
	},
	{
		"input": "sc://?",
		"base": "about:blank",
		"href": "sc://?",
		"protocol": "sc:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "",
		"search": "",
		"hash": ""
	},
	{
		"input": "sc://#",
		"base": "about:blank",
		"href": "sc://#",
		"protocol": "sc:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "",
		"search": "",
		"hash": ""
	},
	{
		"input": "///",
		"base": "sc://x/",
		"href": "sc:///",
		"protocol": "sc:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	{
		"input": "////",
		"base": "sc://x/",
		"href": "sc:////",
		"protocol": "sc:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "//",
		"search": "",
		"hash": ""
	},
	{
		"input": "////x/",
		"base": "sc://x/",
		"href": "sc:////x/",
		"protocol": "sc:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "//x/",
		"search": "",
		"hash": ""
	}
];

runTests(tests);
