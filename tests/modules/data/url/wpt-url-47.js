/*---
description: https://github.com/web-platform-tests/wpt/url/resources/urltestdata.json
flags: [module]
---*/

import { runTests } from "./url_FIXTURE.js";

const tests = [
	{
		"input": "http://[1:0:1:0:1:0:1:0]",
		"base": "about:blank",
		"href": "http://[1:0:1:0:1:0:1:0]/",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "[1:0:1:0:1:0:1:0]",
		"hostname": "[1:0:1:0:1:0:1:0]",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	"Percent-encoded query and fragment",
	{
		"input": "http://example.org/test?\"",
		"base": "about:blank",
		"href": "http://example.org/test?%22",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "example.org",
		"hostname": "example.org",
		"port": "",
		"pathname": "/test",
		"search": "?%22",
		"hash": ""
	},
	{
		"input": "http://example.org/test?#",
		"base": "about:blank",
		"href": "http://example.org/test?#",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "example.org",
		"hostname": "example.org",
		"port": "",
		"pathname": "/test",
		"search": "",
		"hash": ""
	},
	{
		"input": "http://example.org/test?<",
		"base": "about:blank",
		"href": "http://example.org/test?%3C",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "example.org",
		"hostname": "example.org",
		"port": "",
		"pathname": "/test",
		"search": "?%3C",
		"hash": ""
	},
	{
		"input": "http://example.org/test?>",
		"base": "about:blank",
		"href": "http://example.org/test?%3E",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "example.org",
		"hostname": "example.org",
		"port": "",
		"pathname": "/test",
		"search": "?%3E",
		"hash": ""
	},
	{
		"input": "http://example.org/test?⌣",
		"base": "about:blank",
		"href": "http://example.org/test?%E2%8C%A3",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "example.org",
		"hostname": "example.org",
		"port": "",
		"pathname": "/test",
		"search": "?%E2%8C%A3",
		"hash": ""
	},
	{
		"input": "http://example.org/test?%23%23",
		"base": "about:blank",
		"href": "http://example.org/test?%23%23",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "example.org",
		"hostname": "example.org",
		"port": "",
		"pathname": "/test",
		"search": "?%23%23",
		"hash": ""
	},
	{
		"input": "http://example.org/test?%GH",
		"base": "about:blank",
		"href": "http://example.org/test?%GH",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "example.org",
		"hostname": "example.org",
		"port": "",
		"pathname": "/test",
		"search": "?%GH",
		"hash": ""
	},
	{
		"input": "http://example.org/test?a#%EF",
		"base": "about:blank",
		"href": "http://example.org/test?a#%EF",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "example.org",
		"hostname": "example.org",
		"port": "",
		"pathname": "/test",
		"search": "?a",
		"hash": "#%EF"
	},
	{
		"input": "http://example.org/test?a#%GH",
		"base": "about:blank",
		"href": "http://example.org/test?a#%GH",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "example.org",
		"hostname": "example.org",
		"port": "",
		"pathname": "/test",
		"search": "?a",
		"hash": "#%GH"
	},
	"URLs that require a non-about:blank base. (Also serve as invalid base tests.)",
	{
		"input": "a",
		"base": "about:blank",
		"failure": true,
		"inputCanBeRelative": true
	},
	{
		"input": "a/",
		"base": "about:blank",
		"failure": true,
		"inputCanBeRelative": true
	},
	{
		"input": "a//",
		"base": "about:blank",
		"failure": true,
		"inputCanBeRelative": true
	}
];

runTests(tests);
