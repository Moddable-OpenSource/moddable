/*---
description: https://github.com/web-platform-tests/wpt/url/resources/urltestdata.json
flags: [module]
---*/

import { runTests } from "./url_FIXTURE.js";

const tests = [
	"# tests from jsdom/whatwg-url designed for code coverage",
	{
		"input": "http://127.0.0.1:10100/relative_import.html",
		"base": "about:blank",
		"href": "http://127.0.0.1:10100/relative_import.html",
		"origin": "http://127.0.0.1:10100",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "127.0.0.1:10100",
		"hostname": "127.0.0.1",
		"port": "10100",
		"pathname": "/relative_import.html",
		"search": "",
		"hash": ""
	},
	{
		"input": "http://facebook.com/?foo=%7B%22abc%22",
		"base": "about:blank",
		"href": "http://facebook.com/?foo=%7B%22abc%22",
		"origin": "http://facebook.com",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "facebook.com",
		"hostname": "facebook.com",
		"port": "",
		"pathname": "/",
		"search": "?foo=%7B%22abc%22",
		"hash": ""
	},
	{
		"input": "https://localhost:3000/jqueryui@1.2.3",
		"base": "about:blank",
		"href": "https://localhost:3000/jqueryui@1.2.3",
		"origin": "https://localhost:3000",
		"protocol": "https:",
		"username": "",
		"password": "",
		"host": "localhost:3000",
		"hostname": "localhost",
		"port": "3000",
		"pathname": "/jqueryui@1.2.3",
		"search": "",
		"hash": ""
	},
	"# tab/LF/CR",
	{
		"input": "h\tt\nt\rp://h\to\ns\rt:9\t0\n0\r0/p\ta\nt\rh?q\tu\ne\rry#f\tr\na\rg",
		"base": "about:blank",
		"href": "http://host:9000/path?query#frag",
		"origin": "http://host:9000",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "host:9000",
		"hostname": "host",
		"port": "9000",
		"pathname": "/path",
		"search": "?query",
		"hash": "#frag"
	},
	"# Stringification of URL.searchParams",
	{
		"input": "?a=b&c=d",
		"base": "http://example.org/foo/bar",
		"href": "http://example.org/foo/bar?a=b&c=d",
		"origin": "http://example.org",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "example.org",
		"hostname": "example.org",
		"port": "",
		"pathname": "/foo/bar",
		"search": "?a=b&c=d",
		"searchParams": "a=b&c=d",
		"hash": ""
	},
	{
		"input": "??a=b&c=d",
		"base": "http://example.org/foo/bar",
		"href": "http://example.org/foo/bar??a=b&c=d",
		"origin": "http://example.org",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "example.org",
		"hostname": "example.org",
		"port": "",
		"pathname": "/foo/bar",
		"search": "??a=b&c=d",
		"searchParams": "%3Fa=b&c=d",
		"hash": ""
	},
	"# Scheme only",
	{
		"input": "http:",
		"base": "http://example.org/foo/bar",
		"href": "http://example.org/foo/bar",
		"origin": "http://example.org",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "example.org",
		"hostname": "example.org",
		"port": "",
		"pathname": "/foo/bar",
		"search": "",
		"searchParams": "",
		"hash": ""
	},
	{
		"input": "http:",
		"base": "https://example.org/foo/bar",
		"failure": true
	},
	{
		"input": "sc:",
		"base": "https://example.org/foo/bar",
		"href": "sc:",
		"origin": "null",
		"protocol": "sc:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "",
		"search": "",
		"searchParams": "",
		"hash": ""
	},
	"# Percent encoding of fragments",
	{
		"input": "http://foo.bar/baz?qux#foo\bbar",
		"base": "about:blank",
		"href": "http://foo.bar/baz?qux#foo%08bar",
		"origin": "http://foo.bar",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "foo.bar",
		"hostname": "foo.bar",
		"port": "",
		"pathname": "/baz",
		"search": "?qux",
		"searchParams": "qux=",
		"hash": "#foo%08bar"
	}
];

runTests(tests);
