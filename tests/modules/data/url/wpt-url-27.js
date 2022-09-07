/*---
description: https://github.com/web-platform-tests/wpt/url/resources/urltestdata.json
flags: [module]
---*/

import { runTests } from "./url_FIXTURE.js";

const tests = [
	{
		"input": "http://a\u0019b/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://a\u001ab/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://a\u001bb/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://a\u001cb/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://a\u001db/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://a\u001eb/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://a\u001fb/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://a b/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://a%b/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://a<b",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://a>b",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://a[b/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://a]b/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://a^b",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://a|b/",
		"base": "about:blank",
		"failure": true
	}
];

runTests(tests);
