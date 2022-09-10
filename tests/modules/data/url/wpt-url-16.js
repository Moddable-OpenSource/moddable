/*---
description: https://github.com/web-platform-tests/wpt/url/resources/urltestdata.json
flags: [module]
---*/

import { runTests } from "./url_FIXTURE.js";

const tests = [
	{
		"input": "http:a:@www.example.com",
		"base": "about:blank",
		"href": "http://a@www.example.com/",
		"origin": "http://www.example.com",
		"protocol": "http:",
		"username": "a",
		"password": "",
		"host": "www.example.com",
		"hostname": "www.example.com",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	{
		"input": "http:/a:@www.example.com",
		"base": "about:blank",
		"href": "http://a@www.example.com/",
		"origin": "http://www.example.com",
		"protocol": "http:",
		"username": "a",
		"password": "",
		"host": "www.example.com",
		"hostname": "www.example.com",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	{
		"input": "http://a:@www.example.com",
		"base": "about:blank",
		"href": "http://a@www.example.com/",
		"origin": "http://www.example.com",
		"protocol": "http:",
		"username": "a",
		"password": "",
		"host": "www.example.com",
		"hostname": "www.example.com",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	{
		"input": "http://www.@pple.com",
		"base": "about:blank",
		"href": "http://www.@pple.com/",
		"origin": "http://pple.com",
		"protocol": "http:",
		"username": "www.",
		"password": "",
		"host": "pple.com",
		"hostname": "pple.com",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	{
		"input": "http:@:www.example.com",
		"base": "about:blank",
		"failure": true,
		"inputCanBeRelative": true
	},
	{
		"input": "http:/@:www.example.com",
		"base": "about:blank",
		"failure": true,
		"inputCanBeRelative": true
	},
	{
		"input": "http://@:www.example.com",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://:@www.example.com",
		"base": "about:blank",
		"href": "http://www.example.com/",
		"origin": "http://www.example.com",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "www.example.com",
		"hostname": "www.example.com",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	"# Others",
	{
		"input": "/",
		"base": "http://www.example.com/test",
		"href": "http://www.example.com/",
		"origin": "http://www.example.com",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "www.example.com",
		"hostname": "www.example.com",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	{
		"input": "/test.txt",
		"base": "http://www.example.com/test",
		"href": "http://www.example.com/test.txt",
		"origin": "http://www.example.com",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "www.example.com",
		"hostname": "www.example.com",
		"port": "",
		"pathname": "/test.txt",
		"search": "",
		"hash": ""
	},
	{
		"input": ".",
		"base": "http://www.example.com/test",
		"href": "http://www.example.com/",
		"origin": "http://www.example.com",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "www.example.com",
		"hostname": "www.example.com",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	{
		"input": "..",
		"base": "http://www.example.com/test",
		"href": "http://www.example.com/",
		"origin": "http://www.example.com",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "www.example.com",
		"hostname": "www.example.com",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	{
		"input": "test.txt",
		"base": "http://www.example.com/test",
		"href": "http://www.example.com/test.txt",
		"origin": "http://www.example.com",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "www.example.com",
		"hostname": "www.example.com",
		"port": "",
		"pathname": "/test.txt",
		"search": "",
		"hash": ""
	},
	{
		"input": "./test.txt",
		"base": "http://www.example.com/test",
		"href": "http://www.example.com/test.txt",
		"origin": "http://www.example.com",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "www.example.com",
		"hostname": "www.example.com",
		"port": "",
		"pathname": "/test.txt",
		"search": "",
		"hash": ""
	}
];

runTests(tests);
