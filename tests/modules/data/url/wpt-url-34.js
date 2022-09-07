/*---
description: https://github.com/web-platform-tests/wpt/url/resources/urltestdata.json
flags: [module]
---*/

import { runTests } from "./url_FIXTURE.js";

const tests = [
	{
		"input": "http://10000000000",
		"base": "http://other.com/",
		"failure": true
	},
	{
		"input": "http://10000000000.com",
		"base": "http://other.com/",
		"href": "http://10000000000.com/",
		"origin": "http://10000000000.com",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "10000000000.com",
		"hostname": "10000000000.com",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	{
		"input": "http://4294967295",
		"base": "http://other.com/",
		"href": "http://255.255.255.255/",
		"origin": "http://255.255.255.255",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "255.255.255.255",
		"hostname": "255.255.255.255",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	{
		"input": "http://4294967296",
		"base": "http://other.com/",
		"failure": true
	},
	{
		"input": "http://0xffffffff",
		"base": "http://other.com/",
		"href": "http://255.255.255.255/",
		"origin": "http://255.255.255.255",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "255.255.255.255",
		"hostname": "255.255.255.255",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	{
		"input": "http://0xffffffff1",
		"base": "http://other.com/",
		"failure": true
	},
	{
		"input": "http://256.256.256.256",
		"base": "http://other.com/",
		"failure": true
	},
	{
		"input": "https://0x.0x.0",
		"base": "about:blank",
		"href": "https://0.0.0.0/",
		"origin": "https://0.0.0.0",
		"protocol": "https:",
		"username": "",
		"password": "",
		"host": "0.0.0.0",
		"hostname": "0.0.0.0",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	"More IPv4 parsing (via https://github.com/jsdom/whatwg-url/issues/92)",
	{
		"input": "https://0x100000000/test",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "https://256.0.0.1/test",
		"base": "about:blank",
		"failure": true
	},
	"# file URLs containing percent-encoded Windows drive letters (shouldn't work)",
	{
		"input": "file:///C%3A/",
		"base": "about:blank",
		"href": "file:///C%3A/",
		"protocol": "file:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/C%3A/",
		"search": "",
		"hash": ""
	},
	{
		"input": "file:///C%7C/",
		"base": "about:blank",
		"href": "file:///C%7C/",
		"protocol": "file:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/C%7C/",
		"search": "",
		"hash": ""
	},
	{
		"input": "file://%43%3A",
		"base": "about:blank",
		"failure": true
	}
];

runTests(tests);
