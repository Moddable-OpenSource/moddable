/*---
description: https://github.com/web-platform-tests/wpt/url/resources/urltestdata.json
flags: [module]
---*/

import { runTests } from "./url_FIXTURE.js";

const tests = [
	{
		"input": "http://ho%09st/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://ho%0Ast/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://ho%0Bst/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://ho%0Cst/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://ho%0Dst/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://ho%0Est/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://ho%0Fst/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://ho%10st/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://ho%11st/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://ho%12st/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://ho%13st/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://ho%14st/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://ho%15st/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://ho%16st/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://ho%17st/",
		"base": "about:blank",
		"failure": true
	}
];

runTests(tests);
