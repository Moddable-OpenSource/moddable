/*---
description: https://github.com/web-platform-tests/wpt/url/resources/urltestdata.json
flags: [module]
---*/

import { runTests } from "./url_FIXTURE.js";

const tests = [
	"Last component looks like a number, but not valid IPv4",
	{
		"input": "http://1.2.3.4.5",
		"base": "http://other.com/",
		"failure": true
	},
	{
		"input": "http://1.2.3.4.5.",
		"base": "http://other.com/",
		"failure": true
	},
	{
		"input": "http://0..0x300/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://0..0x300./",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://256.256.256.256.256",
		"base": "http://other.com/",
		"failure": true
	},
	{
		"input": "http://256.256.256.256.256.",
		"base": "http://other.com/",
		"failure": true
	},
	{
		"input": "http://1.2.3.08",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://1.2.3.08.",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://1.2.3.09",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://09.2.3.4",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://09.2.3.4.",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://01.2.3.4.5",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://01.2.3.4.5.",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://0x100.2.3.4",
		"base": "about:blank",
		"failure": true
	}
];

runTests(tests);
