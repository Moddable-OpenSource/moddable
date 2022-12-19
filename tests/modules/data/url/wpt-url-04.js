/*---
description: https://github.com/web-platform-tests/wpt/url/resources/urltestdata.json
flags: [module]
---*/

import { runTests } from "./url_FIXTURE.js";

const tests = [
	{
		"input": "//foo/bar",
		"base": "http://example.org/foo/bar",
		"href": "http://foo/bar",
		"origin": "http://foo",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "foo",
		"hostname": "foo",
		"port": "",
		"pathname": "/bar",
		"search": "",
		"hash": ""
	},
	{
		"input": "http://foo/path;a??e#f#g",
		"base": "http://example.org/foo/bar",
		"href": "http://foo/path;a??e#f#g",
		"origin": "http://foo",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "foo",
		"hostname": "foo",
		"port": "",
		"pathname": "/path;a",
		"search": "??e",
		"hash": "#f#g"
	},
	{
		"input": "http://foo/abcd?efgh?ijkl",
		"base": "http://example.org/foo/bar",
		"href": "http://foo/abcd?efgh?ijkl",
		"origin": "http://foo",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "foo",
		"hostname": "foo",
		"port": "",
		"pathname": "/abcd",
		"search": "?efgh?ijkl",
		"hash": ""
	},
	{
		"input": "http://foo/abcd#foo?bar",
		"base": "http://example.org/foo/bar",
		"href": "http://foo/abcd#foo?bar",
		"origin": "http://foo",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "foo",
		"hostname": "foo",
		"port": "",
		"pathname": "/abcd",
		"search": "",
		"hash": "#foo?bar"
	},
	{
		"input": "[61:24:74]:98",
		"base": "http://example.org/foo/bar",
		"href": "http://example.org/foo/[61:24:74]:98",
		"origin": "http://example.org",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "example.org",
		"hostname": "example.org",
		"port": "",
		"pathname": "/foo/[61:24:74]:98",
		"search": "",
		"hash": ""
	},
	{
		"input": "http:[61:27]/:foo",
		"base": "http://example.org/foo/bar",
		"href": "http://example.org/foo/[61:27]/:foo",
		"origin": "http://example.org",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "example.org",
		"hostname": "example.org",
		"port": "",
		"pathname": "/foo/[61:27]/:foo",
		"search": "",
		"hash": ""
	},
	{
		"input": "http://[1::2]:3:4",
		"base": "http://example.org/foo/bar",
		"failure": true
	},
	{
		"input": "http://2001::1",
		"base": "http://example.org/foo/bar",
		"failure": true
	},
	{
		"input": "http://2001::1]",
		"base": "http://example.org/foo/bar",
		"failure": true
	},
	{
		"input": "http://2001::1]:80",
		"base": "http://example.org/foo/bar",
		"failure": true
	},
	{
		"input": "http://[2001::1]",
		"base": "http://example.org/foo/bar",
		"href": "http://[2001::1]/",
		"origin": "http://[2001::1]",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "[2001::1]",
		"hostname": "[2001::1]",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	{
		"input": "http://[::127.0.0.1]",
		"base": "http://example.org/foo/bar",
		"href": "http://[::7f00:1]/",
		"origin": "http://[::7f00:1]",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "[::7f00:1]",
		"hostname": "[::7f00:1]",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	{
		"input": "http://[0:0:0:0:0:0:13.1.68.3]",
		"base": "http://example.org/foo/bar",
		"href": "http://[::d01:4403]/",
		"origin": "http://[::d01:4403]",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "[::d01:4403]",
		"hostname": "[::d01:4403]",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	{
		"input": "http://[2001::1]:80",
		"base": "http://example.org/foo/bar",
		"href": "http://[2001::1]/",
		"origin": "http://[2001::1]",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "[2001::1]",
		"hostname": "[2001::1]",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	{
		"input": "http:/example.com/",
		"base": "http://example.org/foo/bar",
		"href": "http://example.org/example.com/",
		"origin": "http://example.org",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "example.org",
		"hostname": "example.org",
		"port": "",
		"pathname": "/example.com/",
		"search": "",
		"hash": ""
	}
];

runTests(tests);
