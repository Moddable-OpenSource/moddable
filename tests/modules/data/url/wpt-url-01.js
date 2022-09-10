/*---
description: https://github.com/web-platform-tests/wpt/url/resources/urltestdata.json
flags: [module]
---*/

import { runTests } from "./url_FIXTURE.js";

const tests = [
	{
		"input": "http://f:00000000000000/c",
		"base": "http://example.org/foo/bar",
		"href": "http://f:0/c",
		"origin": "http://f:0",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "f:0",
		"hostname": "f",
		"port": "0",
		"pathname": "/c",
		"search": "",
		"hash": ""
	},
	{
		"input": "http://f:00000000000000000000080/c",
		"base": "http://example.org/foo/bar",
		"href": "http://f/c",
		"origin": "http://f",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "f",
		"hostname": "f",
		"port": "",
		"pathname": "/c",
		"search": "",
		"hash": ""
	},
	{
		"input": "http://f:b/c",
		"base": "http://example.org/foo/bar",
		"failure": true
	},
	{
		"input": "http://f: /c",
		"base": "http://example.org/foo/bar",
		"failure": true
	},
	{
		"input": "http://f:\n/c",
		"base": "http://example.org/foo/bar",
		"href": "http://f/c",
		"origin": "http://f",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "f",
		"hostname": "f",
		"port": "",
		"pathname": "/c",
		"search": "",
		"hash": ""
	},
	{
		"input": "http://f:fifty-two/c",
		"base": "http://example.org/foo/bar",
		"failure": true
	},
	{
		"input": "http://f:999999/c",
		"base": "http://example.org/foo/bar",
		"failure": true
	},
	{
		"input": "non-special://f:999999/c",
		"base": "http://example.org/foo/bar",
		"failure": true
	},
	{
		"input": "http://f: 21 / b ? d # e ",
		"base": "http://example.org/foo/bar",
		"failure": true
	},
	{
		"input": "",
		"base": "http://example.org/foo/bar",
		"href": "http://example.org/foo/bar",
		"origin": "http://example.org",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "example.org",
		"hostname": "example.org",
		"port": "",
		"pathname": "/foo/bar",
		"search": "",
		"hash": ""
	},
	{
		"input": "  \t",
		"base": "http://example.org/foo/bar",
		"href": "http://example.org/foo/bar",
		"origin": "http://example.org",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "example.org",
		"hostname": "example.org",
		"port": "",
		"pathname": "/foo/bar",
		"search": "",
		"hash": ""
	},
	{
		"input": ":foo.com/",
		"base": "http://example.org/foo/bar",
		"href": "http://example.org/foo/:foo.com/",
		"origin": "http://example.org",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "example.org",
		"hostname": "example.org",
		"port": "",
		"pathname": "/foo/:foo.com/",
		"search": "",
		"hash": ""
	},
	{
		"input": ":foo.com\\",
		"base": "http://example.org/foo/bar",
		"href": "http://example.org/foo/:foo.com/",
		"origin": "http://example.org",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "example.org",
		"hostname": "example.org",
		"port": "",
		"pathname": "/foo/:foo.com/",
		"search": "",
		"hash": ""
	},
	{
		"input": ":",
		"base": "http://example.org/foo/bar",
		"href": "http://example.org/foo/:",
		"origin": "http://example.org",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "example.org",
		"hostname": "example.org",
		"port": "",
		"pathname": "/foo/:",
		"search": "",
		"hash": ""
	},
	{
		"input": ":a",
		"base": "http://example.org/foo/bar",
		"href": "http://example.org/foo/:a",
		"origin": "http://example.org",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "example.org",
		"hostname": "example.org",
		"port": "",
		"pathname": "/foo/:a",
		"search": "",
		"hash": ""
	}
];

runTests(tests);
