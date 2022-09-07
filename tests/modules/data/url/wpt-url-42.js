/*---
description: https://github.com/web-platform-tests/wpt/url/resources/urltestdata.json
flags: [module]
---*/

import { runTests } from "./url_FIXTURE.js";

const tests = [
	"# IPv6 tests",
	{
		"input": "http://[1:0::]",
		"base": "http://example.net/",
		"href": "http://[1::]/",
		"origin": "http://[1::]",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "[1::]",
		"hostname": "[1::]",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	{
		"input": "http://[0:1:2:3:4:5:6:7:8]",
		"base": "http://example.net/",
		"failure": true
	},
	{
		"input": "https://[0::0::0]",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "https://[0:.0]",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "https://[0:0:]",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "https://[0:1:2:3:4:5:6:7.0.0.0.1]",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "https://[0:1.00.0.0.0]",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "https://[0:1.290.0.0.0]",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "https://[0:1.23.23]",
		"base": "about:blank",
		"failure": true
	},
	"# Empty host",
	{
		"input": "http://?",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://#",
		"base": "about:blank",
		"failure": true
	},
	"Port overflow (2^32 + 81)",
	{
		"input": "http://f:4294967377/c",
		"base": "http://example.org/",
		"failure": true
	}
];

runTests(tests);
