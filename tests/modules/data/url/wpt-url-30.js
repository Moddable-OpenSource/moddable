/*---
description: https://github.com/web-platform-tests/wpt/url/resources/urltestdata.json
flags: [module]
---*/

import { runTests } from "./url_FIXTURE.js";

const tests = [
	{
		"input": "http://ho%18st/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://ho%19st/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://ho%1Ast/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://ho%1Bst/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://ho%1Cst/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://ho%1Dst/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://ho%1Est/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://ho%1Fst/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://ho%20st/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://ho%23st/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://ho%25st/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://ho%2Fst/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://ho%3Ast/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://ho%3Cst/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://ho%3Est/",
		"base": "about:blank",
		"failure": true
	}
];

runTests(tests);
