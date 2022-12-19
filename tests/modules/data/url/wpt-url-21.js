/*---
description: https://github.com/web-platform-tests/wpt/url/resources/urltestdata.json
flags: [module]
---*/

import { runTests } from "./url_FIXTURE.js";

const tests = [
	"# Credentials in base",
	{
		"input": "/some/path",
		"base": "http://user@example.org/smth",
		"href": "http://user@example.org/some/path",
		"origin": "http://example.org",
		"protocol": "http:",
		"username": "user",
		"password": "",
		"host": "example.org",
		"hostname": "example.org",
		"port": "",
		"pathname": "/some/path",
		"search": "",
		"hash": ""
	},
	{
		"input": "",
		"base": "http://user:pass@example.org:21/smth",
		"href": "http://user:pass@example.org:21/smth",
		"origin": "http://example.org:21",
		"protocol": "http:",
		"username": "user",
		"password": "pass",
		"host": "example.org:21",
		"hostname": "example.org",
		"port": "21",
		"pathname": "/smth",
		"search": "",
		"hash": ""
	},
	{
		"input": "/some/path",
		"base": "http://user:pass@example.org:21/smth",
		"href": "http://user:pass@example.org:21/some/path",
		"origin": "http://example.org:21",
		"protocol": "http:",
		"username": "user",
		"password": "pass",
		"host": "example.org:21",
		"hostname": "example.org",
		"port": "21",
		"pathname": "/some/path",
		"search": "",
		"hash": ""
	},
	"# a set of tests designed by zcorpan for relative URLs with unknown schemes",
	{
		"input": "i",
		"base": "sc:sd",
		"failure": true
	},
	{
		"input": "i",
		"base": "sc:sd/sd",
		"failure": true
	},
	{
		"input": "i",
		"base": "sc:/pa/pa",
		"href": "sc:/pa/i",
		"origin": "null",
		"protocol": "sc:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/pa/i",
		"search": "",
		"hash": ""
	},
	{
		"input": "i",
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
		"input": "i",
		"base": "sc:///pa/pa",
		"href": "sc:///pa/i",
		"origin": "null",
		"protocol": "sc:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/pa/i",
		"search": "",
		"hash": ""
	},
	{
		"input": "../i",
		"base": "sc:sd",
		"failure": true
	},
	{
		"input": "../i",
		"base": "sc:sd/sd",
		"failure": true
	},
	{
		"input": "../i",
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
		"input": "../i",
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
		"input": "../i",
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
	}
];

runTests(tests);
