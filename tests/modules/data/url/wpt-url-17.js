/*---
description: https://github.com/web-platform-tests/wpt/url/resources/urltestdata.json
flags: [module]
---*/

import { runTests } from "./url_FIXTURE.js";

const tests = [
	{
		"input": "../test.txt",
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
		"input": "../aaa/test.txt",
		"base": "http://www.example.com/test",
		"href": "http://www.example.com/aaa/test.txt",
		"origin": "http://www.example.com",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "www.example.com",
		"hostname": "www.example.com",
		"port": "",
		"pathname": "/aaa/test.txt",
		"search": "",
		"hash": ""
	},
	{
		"input": "../../test.txt",
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
		"input": "中/test.txt",
		"base": "http://www.example.com/test",
		"href": "http://www.example.com/%E4%B8%AD/test.txt",
		"origin": "http://www.example.com",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "www.example.com",
		"hostname": "www.example.com",
		"port": "",
		"pathname": "/%E4%B8%AD/test.txt",
		"search": "",
		"hash": ""
	},
	{
		"input": "http://www.example2.com",
		"base": "http://www.example.com/test",
		"href": "http://www.example2.com/",
		"origin": "http://www.example2.com",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "www.example2.com",
		"hostname": "www.example2.com",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	{
		"input": "//www.example2.com",
		"base": "http://www.example.com/test",
		"href": "http://www.example2.com/",
		"origin": "http://www.example2.com",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "www.example2.com",
		"hostname": "www.example2.com",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	{
		"input": "file:...",
		"base": "http://www.example.com/test",
		"href": "file:///...",
		"protocol": "file:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/...",
		"search": "",
		"hash": ""
	},
	{
		"input": "file:..",
		"base": "http://www.example.com/test",
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
	{
		"input": "file:a",
		"base": "http://www.example.com/test",
		"href": "file:///a",
		"protocol": "file:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/a",
		"search": "",
		"hash": ""
	},
	"# Based on http://trac.webkit.org/browser/trunk/LayoutTests/fast/url/host.html",
	"Basic canonicalization, uppercase should be converted to lowercase",
	{
		"input": "http://ExAmPlE.CoM",
		"base": "http://other.com/",
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
		"input": "http://example example.com",
		"base": "http://other.com/",
		"failure": true
	},
	{
		"input": "http://Goo%20 goo%7C|.com",
		"base": "http://other.com/",
		"failure": true
	},
	{
		"input": "http://[]",
		"base": "http://other.com/",
		"failure": true
	}
];

runTests(tests);
