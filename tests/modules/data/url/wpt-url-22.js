/*---
description: https://github.com/web-platform-tests/wpt/url/resources/urltestdata.json
flags: [module]
---*/

import { runTests } from "./url_FIXTURE.js";

const tests = [
	{
		"input": "/i",
		"base": "sc:sd",
		"failure": true
	},
	{
		"input": "/i",
		"base": "sc:sd/sd",
		"failure": true
	},
	{
		"input": "/i",
		"base": "sc:/pa/pa",
		"href": "sc:/i",
		"origin": "null",
		"protocol": "sc:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/i",
		"search": "",
		"hash": ""
	},
	{
		"input": "/i",
		"base": "sc://ho/pa",
		"href": "sc://ho/i",
		"origin": "null",
		"protocol": "sc:",
		"username": "",
		"password": "",
		"host": "ho",
		"hostname": "ho",
		"port": "",
		"pathname": "/i",
		"search": "",
		"hash": ""
	},
	{
		"input": "/i",
		"base": "sc:///pa/pa",
		"href": "sc:///i",
		"origin": "null",
		"protocol": "sc:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/i",
		"search": "",
		"hash": ""
	},
	{
		"input": "?i",
		"base": "sc:sd",
		"failure": true
	},
	{
		"input": "?i",
		"base": "sc:sd/sd",
		"failure": true
	},
	{
		"input": "?i",
		"base": "sc:/pa/pa",
		"href": "sc:/pa/pa?i",
		"origin": "null",
		"protocol": "sc:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/pa/pa",
		"search": "?i",
		"hash": ""
	},
	{
		"input": "?i",
		"base": "sc://ho/pa",
		"href": "sc://ho/pa?i",
		"origin": "null",
		"protocol": "sc:",
		"username": "",
		"password": "",
		"host": "ho",
		"hostname": "ho",
		"port": "",
		"pathname": "/pa",
		"search": "?i",
		"hash": ""
	},
	{
		"input": "?i",
		"base": "sc:///pa/pa",
		"href": "sc:///pa/pa?i",
		"origin": "null",
		"protocol": "sc:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/pa/pa",
		"search": "?i",
		"hash": ""
	},
	{
		"input": "#i",
		"base": "sc:sd",
		"href": "sc:sd#i",
		"origin": "null",
		"protocol": "sc:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "sd",
		"search": "",
		"hash": "#i"
	},
	{
		"input": "#i",
		"base": "sc:sd/sd",
		"href": "sc:sd/sd#i",
		"origin": "null",
		"protocol": "sc:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "sd/sd",
		"search": "",
		"hash": "#i"
	},
	{
		"input": "#i",
		"base": "sc:/pa/pa",
		"href": "sc:/pa/pa#i",
		"origin": "null",
		"protocol": "sc:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/pa/pa",
		"search": "",
		"hash": "#i"
	},
	{
		"input": "#i",
		"base": "sc://ho/pa",
		"href": "sc://ho/pa#i",
		"origin": "null",
		"protocol": "sc:",
		"username": "",
		"password": "",
		"host": "ho",
		"hostname": "ho",
		"port": "",
		"pathname": "/pa",
		"search": "",
		"hash": "#i"
	},
	{
		"input": "#i",
		"base": "sc:///pa/pa",
		"href": "sc:///pa/pa#i",
		"origin": "null",
		"protocol": "sc:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/pa/pa",
		"search": "",
		"hash": "#i"
	}
];

runTests(tests);
