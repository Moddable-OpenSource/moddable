/*---
description: https://github.com/web-platform-tests/wpt/url/resources/urltestdata.json
flags: [module]
---*/

import { runTests } from "./url_FIXTURE.js";

const tests = [
	{
		"input": "http://ho%3Fst/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://ho%40st/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://ho%5Bst/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://ho%5Cst/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://ho%5Dst/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://ho%7Cst/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://ho%7Fst/",
		"base": "about:blank",
		"failure": true
	},
	"Allowed host/domain code points",
	{
		"input": "http://!\"$&'()*+,-.;=_`{}~/",
		"base": "about:blank",
		"href": "http://!\"$&'()*+,-.;=_`{}~/",
		"origin": "http://!\"$&'()*+,-.;=_`{}~",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "!\"$&'()*+,-.;=_`{}~",
		"hostname": "!\"$&'()*+,-.;=_`{}~",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	{
		"input": "sc://\u0001\u0002\u0003\u0004\u0005\u0006\u0007\b\u000b\f\u000e\u000f\u0010\u0011\u0012\u0013\u0014\u0015\u0016\u0017\u0018\u0019\u001a\u001b\u001c\u001d\u001e\u001f!\"$%&'()*+,-.;=_`{}~/",
		"base": "about:blank",
		"href": "sc://%01%02%03%04%05%06%07%08%0B%0C%0E%0F%10%11%12%13%14%15%16%17%18%19%1A%1B%1C%1D%1E%1F%7F!\"$%&'()*+,-.;=_`{}~/",
		"origin": "null",
		"protocol": "sc:",
		"username": "",
		"password": "",
		"host": "%01%02%03%04%05%06%07%08%0B%0C%0E%0F%10%11%12%13%14%15%16%17%18%19%1A%1B%1C%1D%1E%1F%7F!\"$%&'()*+,-.;=_`{}~",
		"hostname": "%01%02%03%04%05%06%07%08%0B%0C%0E%0F%10%11%12%13%14%15%16%17%18%19%1A%1B%1C%1D%1E%1F%7F!\"$%&'()*+,-.;=_`{}~",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	"# Hosts and percent-encoding",
	{
		"input": "ftp://example.com%80/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "ftp://example.com%A0/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "https://example.com%80/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "https://example.com%A0/",
		"base": "about:blank",
		"failure": true
	}
];

runTests(tests);
