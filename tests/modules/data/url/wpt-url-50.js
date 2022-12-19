/*---
description: https://github.com/web-platform-tests/wpt/url/resources/urltestdata.json
flags: [module]
---*/

import { runTests } from "./url_FIXTURE.js";

const tests = [
	{
		"input": "wss://!\"$&'()*+,-.;=_`{}~/",
		"base": "about:blank",
		"hash": "",
		"host": "!\"$&'()*+,-.;=_`{}~",
		"hostname": "!\"$&'()*+,-.;=_`{}~",
		"href": "wss://!\"$&'()*+,-.;=_`{}~/",
		"origin": "wss://!\"$&'()*+,-.;=_`{}~",
		"password": "",
		"pathname": "/",
		"port": "",
		"protocol": "wss:",
		"search": "",
		"username": ""
	},
	{
		"input": "foo://host/ !\"$%&'()*+,-./:;<=>@[\\]^_`{|}~",
		"base": "about:blank",
		"hash": "",
		"host": "host",
		"hostname": "host",
		"href": "foo://host/%20!%22$%&'()*+,-./:;%3C=%3E@[\\]^_%60%7B|%7D~",
		"origin": "null",
		"password": "",
		"pathname": "/%20!%22$%&'()*+,-./:;%3C=%3E@[\\]^_%60%7B|%7D~",
		"port": "",
		"protocol": "foo:",
		"search": "",
		"username": ""
	},
	{
		"input": "wss://host/ !\"$%&'()*+,-./:;<=>@[\\]^_`{|}~",
		"base": "about:blank",
		"hash": "",
		"host": "host",
		"hostname": "host",
		"href": "wss://host/%20!%22$%&'()*+,-./:;%3C=%3E@[/]^_%60%7B|%7D~",
		"origin": "wss://host",
		"password": "",
		"pathname": "/%20!%22$%&'()*+,-./:;%3C=%3E@[/]^_%60%7B|%7D~",
		"port": "",
		"protocol": "wss:",
		"search": "",
		"username": ""
	},
	{
		"input": "foo://host/dir/? !\"$%&'()*+,-./:;<=>?@[\\]^_`{|}~",
		"base": "about:blank",
		"hash": "",
		"host": "host",
		"hostname": "host",
		"href": "foo://host/dir/?%20!%22$%&'()*+,-./:;%3C=%3E?@[\\]^_`{|}~",
		"origin": "null",
		"password": "",
		"pathname": "/dir/",
		"port": "",
		"protocol": "foo:",
		"search": "?%20!%22$%&'()*+,-./:;%3C=%3E?@[\\]^_`{|}~",
		"username": ""
	},
	{
		"input": "wss://host/dir/? !\"$%&'()*+,-./:;<=>?@[\\]^_`{|}~",
		"base": "about:blank",
		"hash": "",
		"host": "host",
		"hostname": "host",
		"href": "wss://host/dir/?%20!%22$%&%27()*+,-./:;%3C=%3E?@[\\]^_`{|}~",
		"origin": "wss://host",
		"password": "",
		"pathname": "/dir/",
		"port": "",
		"protocol": "wss:",
		"search": "?%20!%22$%&%27()*+,-./:;%3C=%3E?@[\\]^_`{|}~",
		"username": ""
	},
	{
		"input": "foo://host/dir/# !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~",
		"base": "about:blank",
		"hash": "#%20!%22#$%&'()*+,-./:;%3C=%3E?@[\\]^_%60{|}~",
		"host": "host",
		"hostname": "host",
		"href": "foo://host/dir/#%20!%22#$%&'()*+,-./:;%3C=%3E?@[\\]^_%60{|}~",
		"origin": "null",
		"password": "",
		"pathname": "/dir/",
		"port": "",
		"protocol": "foo:",
		"search": "",
		"username": ""
	},
	{
		"input": "wss://host/dir/# !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~",
		"base": "about:blank",
		"hash": "#%20!%22#$%&'()*+,-./:;%3C=%3E?@[\\]^_%60{|}~",
		"host": "host",
		"hostname": "host",
		"href": "wss://host/dir/#%20!%22#$%&'()*+,-./:;%3C=%3E?@[\\]^_%60{|}~",
		"origin": "wss://host",
		"password": "",
		"pathname": "/dir/",
		"port": "",
		"protocol": "wss:",
		"search": "",
		"username": ""
	},
	"Ensure that input schemes are not ignored when resolving non-special URLs",
	{
		"input": "abc:rootless",
		"base": "abc://host/path",
		"hash": "",
		"host": "",
		"hostname": "",
		"href": "abc:rootless",
		"password": "",
		"pathname": "rootless",
		"port": "",
		"protocol": "abc:",
		"search": "",
		"username": ""
	},
	{
		"input": "abc:rootless",
		"base": "abc:/path",
		"hash": "",
		"host": "",
		"hostname": "",
		"href": "abc:rootless",
		"password": "",
		"pathname": "rootless",
		"port": "",
		"protocol": "abc:",
		"search": "",
		"username": ""
	},
	{
		"input": "abc:rootless",
		"base": "abc:path",
		"hash": "",
		"host": "",
		"hostname": "",
		"href": "abc:rootless",
		"password": "",
		"pathname": "rootless",
		"port": "",
		"protocol": "abc:",
		"search": "",
		"username": ""
	},
	{
		"input": "abc:/rooted",
		"base": "abc://host/path",
		"hash": "",
		"host": "",
		"hostname": "",
		"href": "abc:/rooted",
		"password": "",
		"pathname": "/rooted",
		"port": "",
		"protocol": "abc:",
		"search": "",
		"username": ""
	},
	"Empty query and fragment with blank should throw an error",
	{
		"input": "#",
		"base": null,
		"failure": true
	},
	{
		"input": "?",
		"base": null,
		"failure": true
	}
];

runTests(tests);
