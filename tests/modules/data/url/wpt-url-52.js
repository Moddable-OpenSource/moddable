/*---
description: https://github.com/web-platform-tests/wpt/url/resources/urltestdata.json
flags: [module]
---*/

import { runTests } from "./url_FIXTURE.js";

const tests = [
	{
		"input": "http://0x100.2.3.4.",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://0x1.2.3.4.5",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://0x1.2.3.4.5.",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://foo.1.2.3.4",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://foo.1.2.3.4.",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://foo.2.3.4",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://foo.2.3.4.",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://foo.09",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://foo.09.",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://foo.0x4",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://foo.0x4.",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://foo.09..",
		"base": "about:blank",
		"hash": "",
		"host": "foo.09..",
		"hostname": "foo.09..",
		"href": "http://foo.09../",
		"password": "",
		"pathname": "/",
		"port": "",
		"protocol": "http:",
		"search": "",
		"username": ""
	},
	{
		"input": "http://0999999999999999999/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://foo.0x",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "http://foo.0XFfFfFfFfFfFfFfFfFfAcE123",
		"base": "about:blank",
		"failure": true
	}
];

runTests(tests);
