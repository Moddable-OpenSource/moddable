/*---
description: https://github.com/web-platform-tests/wpt/url/resources/urltestdata.json
flags: [module]
---*/

import { runTests } from "./url_FIXTURE.js";

const tests = [
	"Bases that don't fail to parse but fail to be bases",
	{
		"input": "test-a-colon.html",
		"base": "a:",
		"failure": true
	},
	{
		"input": "test-a-colon-b.html",
		"base": "a:b",
		"failure": true
	},
	"Other base URL tests, that must succeed",
	{
		"input": "test-a-colon-slash.html",
		"base": "a:/",
		"href": "a:/test-a-colon-slash.html",
		"protocol": "a:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/test-a-colon-slash.html",
		"search": "",
		"hash": ""
	},
	{
		"input": "test-a-colon-slash-slash.html",
		"base": "a://",
		"href": "a:///test-a-colon-slash-slash.html",
		"protocol": "a:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/test-a-colon-slash-slash.html",
		"search": "",
		"hash": ""
	},
	{
		"input": "test-a-colon-slash-b.html",
		"base": "a:/b",
		"href": "a:/test-a-colon-slash-b.html",
		"protocol": "a:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/test-a-colon-slash-b.html",
		"search": "",
		"hash": ""
	},
	{
		"input": "test-a-colon-slash-slash-b.html",
		"base": "a://b",
		"href": "a://b/test-a-colon-slash-slash-b.html",
		"protocol": "a:",
		"username": "",
		"password": "",
		"host": "b",
		"hostname": "b",
		"port": "",
		"pathname": "/test-a-colon-slash-slash-b.html",
		"search": "",
		"hash": ""
	},
	"Null code point in fragment",
	{
		"input": "http://example.org/test?a#b\u0000c",
		"base": "about:blank",
		"href": "http://example.org/test?a#b%00c",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "example.org",
		"hostname": "example.org",
		"port": "",
		"pathname": "/test",
		"search": "?a",
		"hash": "#b%00c"
	},
	{
		"input": "non-spec://example.org/test?a#b\u0000c",
		"base": "about:blank",
		"href": "non-spec://example.org/test?a#b%00c",
		"protocol": "non-spec:",
		"username": "",
		"password": "",
		"host": "example.org",
		"hostname": "example.org",
		"port": "",
		"pathname": "/test",
		"search": "?a",
		"hash": "#b%00c"
	},
	{
		"input": "non-spec:/test?a#b\u0000c",
		"base": "about:blank",
		"href": "non-spec:/test?a#b%00c",
		"protocol": "non-spec:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/test",
		"search": "?a",
		"hash": "#b%00c"
	},
	"First scheme char - not allowed: https://github.com/whatwg/url/issues/464",
	{
		"input": "10.0.0.7:8080/foo.html",
		"base": "file:///some/dir/bar.html",
		"href": "file:///some/dir/10.0.0.7:8080/foo.html",
		"protocol": "file:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/some/dir/10.0.0.7:8080/foo.html",
		"search": "",
		"hash": ""
	},
	"Subsequent scheme chars - not allowed"
];

runTests(tests);
