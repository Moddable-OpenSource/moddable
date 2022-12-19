/*---
description: https://github.com/web-platform-tests/wpt/url/resources/urltestdata.json
flags: [module]
---*/

import { runTests } from "./url_FIXTURE.js";

const tests = [
	{
		"input": "file://example.net/C:/",
		"base": "about:blank",
		"href": "file://example.net/C:/",
		"protocol": "file:",
		"username": "",
		"password": "",
		"host": "example.net",
		"hostname": "example.net",
		"port": "",
		"pathname": "/C:/",
		"search": "",
		"hash": ""
	},
	{
		"input": "file://1.2.3.4/C:/",
		"base": "about:blank",
		"href": "file://1.2.3.4/C:/",
		"protocol": "file:",
		"username": "",
		"password": "",
		"host": "1.2.3.4",
		"hostname": "1.2.3.4",
		"port": "",
		"pathname": "/C:/",
		"search": "",
		"hash": ""
	},
	{
		"input": "file://[1::8]/C:/",
		"base": "about:blank",
		"href": "file://[1::8]/C:/",
		"protocol": "file:",
		"username": "",
		"password": "",
		"host": "[1::8]",
		"hostname": "[1::8]",
		"port": "",
		"pathname": "/C:/",
		"search": "",
		"hash": ""
	},
	"# Copy the host from the base URL in the following cases",
	{
		"input": "C|/",
		"base": "file://host/",
		"href": "file://host/C:/",
		"protocol": "file:",
		"username": "",
		"password": "",
		"host": "host",
		"hostname": "host",
		"port": "",
		"pathname": "/C:/",
		"search": "",
		"hash": ""
	},
	{
		"input": "/C:/",
		"base": "file://host/",
		"href": "file://host/C:/",
		"protocol": "file:",
		"username": "",
		"password": "",
		"host": "host",
		"hostname": "host",
		"port": "",
		"pathname": "/C:/",
		"search": "",
		"hash": ""
	},
	{
		"input": "file:C:/",
		"base": "file://host/",
		"href": "file://host/C:/",
		"protocol": "file:",
		"username": "",
		"password": "",
		"host": "host",
		"hostname": "host",
		"port": "",
		"pathname": "/C:/",
		"search": "",
		"hash": ""
	},
	{
		"input": "file:/C:/",
		"base": "file://host/",
		"href": "file://host/C:/",
		"protocol": "file:",
		"username": "",
		"password": "",
		"host": "host",
		"hostname": "host",
		"port": "",
		"pathname": "/C:/",
		"search": "",
		"hash": ""
	},
	"# Copy the empty host from the input in the following cases",
	{
		"input": "//C:/",
		"base": "file://host/",
		"href": "file:///C:/",
		"protocol": "file:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/C:/",
		"search": "",
		"hash": ""
	},
	{
		"input": "file://C:/",
		"base": "file://host/",
		"href": "file:///C:/",
		"protocol": "file:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/C:/",
		"search": "",
		"hash": ""
	},
	{
		"input": "///C:/",
		"base": "file://host/",
		"href": "file:///C:/",
		"protocol": "file:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/C:/",
		"search": "",
		"hash": ""
	},
	{
		"input": "file:///C:/",
		"base": "file://host/",
		"href": "file:///C:/",
		"protocol": "file:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/C:/",
		"search": "",
		"hash": ""
	},
	"# Windows drive letter quirk (no host)",
	{
		"input": "file:/C|/",
		"base": "about:blank",
		"href": "file:///C:/",
		"protocol": "file:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/C:/",
		"search": "",
		"hash": ""
	}
];

runTests(tests);
