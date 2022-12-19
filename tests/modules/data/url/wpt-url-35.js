/*---
description: https://github.com/web-platform-tests/wpt/url/resources/urltestdata.json
flags: [module]
---*/

import { runTests } from "./url_FIXTURE.js";

const tests = [
	{
		"input": "file://%43%7C",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "file://%43|",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "file://C%7C",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "file://%43%7C/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "https://%43%7C/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "asdf://%43|/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "asdf://%43%7C/",
		"base": "about:blank",
		"href": "asdf://%43%7C/",
		"origin": "null",
		"protocol": "asdf:",
		"username": "",
		"password": "",
		"host": "%43%7C",
		"hostname": "%43%7C",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	"# file URLs relative to other file URLs (via https://github.com/jsdom/whatwg-url/pull/60)",
	{
		"input": "pix/submit.gif",
		"base": "file:///C:/Users/Domenic/Dropbox/GitHub/tmpvar/jsdom/test/level2/html/files/anchor.html",
		"href": "file:///C:/Users/Domenic/Dropbox/GitHub/tmpvar/jsdom/test/level2/html/files/pix/submit.gif",
		"protocol": "file:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/C:/Users/Domenic/Dropbox/GitHub/tmpvar/jsdom/test/level2/html/files/pix/submit.gif",
		"search": "",
		"hash": ""
	},
	{
		"input": "..",
		"base": "file:///C:/",
		"href": "file:///C:/",
		"protocol": "file:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/C:/",
		"search": "",
		"hash": ""
	},
	{
		"input": "..",
		"base": "file:///",
		"href": "file:///",
		"protocol": "file:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	"# More file URL tests by zcorpan and annevk",
	{
		"input": "/",
		"base": "file:///C:/a/b",
		"href": "file:///C:/",
		"protocol": "file:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/C:/",
		"search": "",
		"hash": ""
	},
	{
		"input": "/",
		"base": "file://h/C:/a/b",
		"href": "file://h/C:/",
		"protocol": "file:",
		"username": "",
		"password": "",
		"host": "h",
		"hostname": "h",
		"port": "",
		"pathname": "/C:/",
		"search": "",
		"hash": ""
	},
	{
		"input": "/",
		"base": "file://h/a/b",
		"href": "file://h/",
		"protocol": "file:",
		"username": "",
		"password": "",
		"host": "h",
		"hostname": "h",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	}
];

runTests(tests);
