/*---
description: https://github.com/web-platform-tests/wpt/url/resources/urltestdata.json
flags: [module]
---*/

import { runTests } from "./url_FIXTURE.js";

const tests = [
	{
		"input": "http://a\u0007b/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://a\bb/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://a\u000bb/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://a\fb/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://a\u000eb/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://a\u000fb/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://a\u0010b/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://a\u0011b/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://a\u0012b/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://a\u0013b/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://a\u0014b/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://a\u0015b/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://a\u0016b/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://a\u0017b/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://a\u0018b/",
		"base": "about:blank",
		"failure": true
	}
];

runTests(tests);
