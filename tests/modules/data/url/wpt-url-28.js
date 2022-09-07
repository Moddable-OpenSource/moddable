/*---
description: https://github.com/web-platform-tests/wpt/url/resources/urltestdata.json
flags: [module]
---*/

import { runTests } from "./url_FIXTURE.js";

const tests = [
	{
		"input": "http://ab/",
		"base": "about:blank",
		"failure": true
	},
	"Forbidden domain codepoints: tabs and newlines are removed during preprocessing",
	{
		"input": "http://ho\tst/",
		"base": "about:blank",
		"hash": "",
		"host": "host",
		"hostname": "host",
		"href": "http://host/",
		"password": "",
		"pathname": "/",
		"port": "",
		"protocol": "http:",
		"search": "",
		"username": ""
	},
	{
		"input": "http://ho\nst/",
		"base": "about:blank",
		"hash": "",
		"host": "host",
		"hostname": "host",
		"href": "http://host/",
		"password": "",
		"pathname": "/",
		"port": "",
		"protocol": "http:",
		"search": "",
		"username": ""
	},
	{
		"input": "http://ho\rst/",
		"base": "about:blank",
		"hash": "",
		"host": "host",
		"hostname": "host",
		"href": "http://host/",
		"password": "",
		"pathname": "/",
		"port": "",
		"protocol": "http:",
		"search": "",
		"username": ""
	},
	"Encoded forbidden domain codepoints in special URLs",
	{
		"input": "http://ho%00st/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://ho%01st/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://ho%02st/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://ho%03st/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://ho%04st/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://ho%05st/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://ho%06st/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://ho%07st/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://ho%08st/",
		"base": "about:blank",
		"failure": true
	}
];

runTests(tests);
