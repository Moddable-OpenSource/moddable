/*---
description: https://github.com/web-platform-tests/wpt/url/resources/urltestdata.json
flags: [module]
---*/

import { runTests } from "./url_FIXTURE.js";

const tests = [
	{
		"input": "sc://a]b/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "sc://a^b",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "sc://a|b/",
		"base": "about:blank",
		"failure": true
	},
	"Forbidden host codepoints: tabs and newlines are removed during preprocessing",
	{
		"input": "foo://ho\tst/",
		"base": "about:blank",
		"hash": "",
		"host": "host",
		"hostname": "host",
		"href": "foo://host/",
		"password": "",
		"pathname": "/",
		"port": "",
		"protocol": "foo:",
		"search": "",
		"username": ""
	},
	{
		"input": "foo://ho\nst/",
		"base": "about:blank",
		"hash": "",
		"host": "host",
		"hostname": "host",
		"href": "foo://host/",
		"password": "",
		"pathname": "/",
		"port": "",
		"protocol": "foo:",
		"search": "",
		"username": ""
	},
	{
		"input": "foo://ho\rst/",
		"base": "about:blank",
		"hash": "",
		"host": "host",
		"hostname": "host",
		"href": "foo://host/",
		"password": "",
		"pathname": "/",
		"port": "",
		"protocol": "foo:",
		"search": "",
		"username": ""
	},
	"Forbidden domain code-points",
	{
		"input": "http://a\u0000b/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://a\u0001b/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://a\u0002b/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://a\u0003b/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://a\u0004b/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://a\u0005b/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://a\u0006b/",
		"base": "about:blank",
		"failure": true
	}
];

runTests(tests);
