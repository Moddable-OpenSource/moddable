/*---
description: https://github.com/web-platform-tests/wpt/url/resources/urltestdata.json
flags: [module]
---*/

import { runTests } from "./url_FIXTURE.js";

const tests = [
	"# unknown scheme with path looking like a password",
	{
		"input": "sc::a@example.net",
		"base": "about:blank",
		"href": "sc::a@example.net",
		"origin": "null",
		"protocol": "sc:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": ":a@example.net",
		"search": "",
		"hash": ""
	},
	"# unknown scheme with bogus percent-encoding",
	{
		"input": "wow:%NBD",
		"base": "about:blank",
		"href": "wow:%NBD",
		"origin": "null",
		"protocol": "wow:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "%NBD",
		"search": "",
		"hash": ""
	},
	{
		"input": "wow:%1G",
		"base": "about:blank",
		"href": "wow:%1G",
		"origin": "null",
		"protocol": "wow:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "%1G",
		"search": "",
		"hash": ""
	},
	"# unknown scheme with non-URL characters",
	{
		"input": "wow:￿",
		"base": "about:blank",
		"href": "wow:%EF%BF%BF",
		"origin": "null",
		"protocol": "wow:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "%EF%BF%BF",
		"search": "",
		"hash": ""
	},
	{
		"input": "http://example.com/\ud800𐟾\udfff﷐﷏﷯ﷰ￾￿?\ud800𐟾\udfff﷐﷏﷯ﷰ￾￿",
		"base": "about:blank",
		"href": "http://example.com/%EF%BF%BD%F0%90%9F%BE%EF%BF%BD%EF%B7%90%EF%B7%8F%EF%B7%AF%EF%B7%B0%EF%BF%BE%EF%BF%BF?%EF%BF%BD%F0%90%9F%BE%EF%BF%BD%EF%B7%90%EF%B7%8F%EF%B7%AF%EF%B7%B0%EF%BF%BE%EF%BF%BF",
		"origin": "http://example.com",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "example.com",
		"hostname": "example.com",
		"port": "",
		"pathname": "/%EF%BF%BD%F0%90%9F%BE%EF%BF%BD%EF%B7%90%EF%B7%8F%EF%B7%AF%EF%B7%B0%EF%BF%BE%EF%BF%BF",
		"search": "?%EF%BF%BD%F0%90%9F%BE%EF%BF%BD%EF%B7%90%EF%B7%8F%EF%B7%AF%EF%B7%B0%EF%BF%BE%EF%BF%BF",
		"hash": ""
	},
	"Forbidden host code points",
	{
		"input": "sc://a\u0000b/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "sc://a b/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "sc://a<b",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "sc://a>b",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "sc://a[b/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "sc://a\\b/",
		"base": "about:blank",
		"failure": true
	}
];

runTests(tests);
