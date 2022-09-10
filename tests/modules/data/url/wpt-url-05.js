/*---
description: https://github.com/web-platform-tests/wpt/url/resources/urltestdata.json
flags: [module]
---*/

import { runTests } from "./url_FIXTURE.js";

const tests = [
	{
		"input": "ftp:/example.com/",
		"base": "http://example.org/foo/bar",
		"href": "ftp://example.com/",
		"origin": "ftp://example.com",
		"protocol": "ftp:",
		"username": "",
		"password": "",
		"host": "example.com",
		"hostname": "example.com",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	{
		"input": "https:/example.com/",
		"base": "http://example.org/foo/bar",
		"href": "https://example.com/",
		"origin": "https://example.com",
		"protocol": "https:",
		"username": "",
		"password": "",
		"host": "example.com",
		"hostname": "example.com",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	{
		"input": "madeupscheme:/example.com/",
		"base": "http://example.org/foo/bar",
		"href": "madeupscheme:/example.com/",
		"origin": "null",
		"protocol": "madeupscheme:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/example.com/",
		"search": "",
		"hash": ""
	},
	{
		"input": "file:/example.com/",
		"base": "http://example.org/foo/bar",
		"href": "file:///example.com/",
		"protocol": "file:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/example.com/",
		"search": "",
		"hash": ""
	},
	{
		"input": "file://example:1/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "file://example:test/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "file://example%/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "file://[example]/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "ftps:/example.com/",
		"base": "http://example.org/foo/bar",
		"href": "ftps:/example.com/",
		"origin": "null",
		"protocol": "ftps:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/example.com/",
		"search": "",
		"hash": ""
	},
	{
		"input": "gopher:/example.com/",
		"base": "http://example.org/foo/bar",
		"href": "gopher:/example.com/",
		"origin": "null",
		"protocol": "gopher:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/example.com/",
		"search": "",
		"hash": ""
	},
	{
		"input": "ws:/example.com/",
		"base": "http://example.org/foo/bar",
		"href": "ws://example.com/",
		"origin": "ws://example.com",
		"protocol": "ws:",
		"username": "",
		"password": "",
		"host": "example.com",
		"hostname": "example.com",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	{
		"input": "wss:/example.com/",
		"base": "http://example.org/foo/bar",
		"href": "wss://example.com/",
		"origin": "wss://example.com",
		"protocol": "wss:",
		"username": "",
		"password": "",
		"host": "example.com",
		"hostname": "example.com",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	{
		"input": "data:/example.com/",
		"base": "http://example.org/foo/bar",
		"href": "data:/example.com/",
		"origin": "null",
		"protocol": "data:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/example.com/",
		"search": "",
		"hash": ""
	},
	{
		"input": "javascript:/example.com/",
		"base": "http://example.org/foo/bar",
		"href": "javascript:/example.com/",
		"origin": "null",
		"protocol": "javascript:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/example.com/",
		"search": "",
		"hash": ""
	},
	{
		"input": "mailto:/example.com/",
		"base": "http://example.org/foo/bar",
		"href": "mailto:/example.com/",
		"origin": "null",
		"protocol": "mailto:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/example.com/",
		"search": "",
		"hash": ""
	}
];

runTests(tests);
