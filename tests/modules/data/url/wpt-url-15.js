/*---
description: https://github.com/web-platform-tests/wpt/url/resources/urltestdata.json
flags: [module]
---*/

import { runTests } from "./url_FIXTURE.js";

const tests = [
	{
		"input": "http://a:b@www.example.com",
		"base": "about:blank",
		"href": "http://a:b@www.example.com/",
		"origin": "http://www.example.com",
		"protocol": "http:",
		"username": "a",
		"password": "b",
		"host": "www.example.com",
		"hostname": "www.example.com",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	{
		"input": "http://@pple.com",
		"base": "about:blank",
		"href": "http://pple.com/",
		"origin": "http://pple.com",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "pple.com",
		"hostname": "pple.com",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	{
		"input": "http::b@www.example.com",
		"base": "about:blank",
		"href": "http://:b@www.example.com/",
		"origin": "http://www.example.com",
		"protocol": "http:",
		"username": "",
		"password": "b",
		"host": "www.example.com",
		"hostname": "www.example.com",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	{
		"input": "http:/:b@www.example.com",
		"base": "about:blank",
		"href": "http://:b@www.example.com/",
		"origin": "http://www.example.com",
		"protocol": "http:",
		"username": "",
		"password": "b",
		"host": "www.example.com",
		"hostname": "www.example.com",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	{
		"input": "http://:b@www.example.com",
		"base": "about:blank",
		"href": "http://:b@www.example.com/",
		"origin": "http://www.example.com",
		"protocol": "http:",
		"username": "",
		"password": "b",
		"host": "www.example.com",
		"hostname": "www.example.com",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	{
		"input": "http:/:@/www.example.com",
		"base": "about:blank",
		"failure": true,
		"inputCanBeRelative": true
	},
	{
		"input": "http://user@/www.example.com",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http:@/www.example.com",
		"base": "about:blank",
		"failure": true,
		"inputCanBeRelative": true
	},
	{
		"input": "http:/@/www.example.com",
		"base": "about:blank",
		"failure": true,
		"inputCanBeRelative": true
	},
	{
		"input": "http://@/www.example.com",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "https:@/www.example.com",
		"base": "about:blank",
		"failure": true,
		"inputCanBeRelative": true
	},
	{
		"input": "http:a:b@/www.example.com",
		"base": "about:blank",
		"failure": true,
		"inputCanBeRelative": true
	},
	{
		"input": "http:/a:b@/www.example.com",
		"base": "about:blank",
		"failure": true,
		"inputCanBeRelative": true
	},
	{
		"input": "http://a:b@/www.example.com",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http::@/www.example.com",
		"base": "about:blank",
		"failure": true,
		"inputCanBeRelative": true
	}
];

runTests(tests);
