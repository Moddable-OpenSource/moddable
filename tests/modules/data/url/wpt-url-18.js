/*---
description: https://github.com/web-platform-tests/wpt/url/resources/urltestdata.json
flags: [module]
---*/

import { runTests } from "./url_FIXTURE.js";

const tests = [
	{
		"input": "http://[:]",
		"base": "http://other.com/",
		"failure": true
	},
	"Leading and trailing C0 control or space",
	{
		"input": "\u0000\u001b\u0004\u0012 http://example.com/\u001f \r ",
		"base": "about:blank",
		"href": "http://example.com/",
		"origin": "http://example.com",
		"protocol": "http:",
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
		"input": "https://x/�?�#�",
		"base": "about:blank",
		"href": "https://x/%EF%BF%BD?%EF%BF%BD#%EF%BF%BD",
		"origin": "https://x",
		"protocol": "https:",
		"username": "",
		"password": "",
		"host": "x",
		"hostname": "x",
		"port": "",
		"pathname": "/%EF%BF%BD",
		"search": "?%EF%BF%BD",
		"hash": "#%EF%BF%BD"
	},
	{
		"input": "sc://faß.ExAmPlE/",
		"base": "about:blank",
		"href": "sc://fa%C3%9F.ExAmPlE/",
		"origin": "null",
		"protocol": "sc:",
		"username": "",
		"password": "",
		"host": "fa%C3%9F.ExAmPlE",
		"hostname": "fa%C3%9F.ExAmPlE",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	"Invalid escaped characters should fail and the percents should be escaped. https://www.w3.org/Bugs/Public/show_bug.cgi?id=24191",
	{
		"input": "http://%zz%66%a.com",
		"base": "http://other.com/",
		"failure": true
	},
	"If we get an invalid character that has been escaped.",
	{
		"input": "http://%25",
		"base": "http://other.com/",
		"failure": true
	},
	{
		"input": "http://hello%00",
		"base": "http://other.com/",
		"failure": true
	},
	"Escaped numbers should be treated like IP addresses if they are.",
	{
		"input": "http://%30%78%63%30%2e%30%32%35%30.01",
		"base": "http://other.com/",
		"href": "http://192.168.0.1/",
		"origin": "http://192.168.0.1",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "192.168.0.1",
		"hostname": "192.168.0.1",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	{
		"input": "http://%30%78%63%30%2e%30%32%35%30.01%2e",
		"base": "http://other.com/",
		"href": "http://192.168.0.1/",
		"origin": "http://192.168.0.1",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "192.168.0.1",
		"hostname": "192.168.0.1",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	{
		"input": "http://192.168.0.257",
		"base": "http://other.com/",
		"failure": true
	},
	"Invalid escaping in hosts causes failure"
];

runTests(tests);
