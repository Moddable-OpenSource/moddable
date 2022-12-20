/*---
description: https://github.com/web-platform-tests/wpt/url/resources/urltestdata.json
flags: [module]
---*/

import { runTests } from "./url_FIXTURE.js";

const tests = [
	"# IPv6 in non-special-URLs",
	{
		"input": "non-special://[1:2:0:0:5:0:0:0]/",
		"base": "about:blank",
		"href": "non-special://[1:2:0:0:5::]/",
		"protocol": "non-special:",
		"username": "",
		"password": "",
		"host": "[1:2:0:0:5::]",
		"hostname": "[1:2:0:0:5::]",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	{
		"input": "non-special://[1:2:0:0:0:0:0:3]/",
		"base": "about:blank",
		"href": "non-special://[1:2::3]/",
		"protocol": "non-special:",
		"username": "",
		"password": "",
		"host": "[1:2::3]",
		"hostname": "[1:2::3]",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	{
		"input": "non-special://[1:2::3]:80/",
		"base": "about:blank",
		"href": "non-special://[1:2::3]:80/",
		"protocol": "non-special:",
		"username": "",
		"password": "",
		"host": "[1:2::3]:80",
		"hostname": "[1:2::3]",
		"port": "80",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	{
		"input": "non-special://[:80/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "blob:d3958f5c-0777-0845-9dcf-2cb28783acaf",
		"base": "about:blank",
		"href": "blob:d3958f5c-0777-0845-9dcf-2cb28783acaf",
		"origin": "null",
		"protocol": "blob:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "d3958f5c-0777-0845-9dcf-2cb28783acaf",
		"search": "",
		"hash": ""
	},
	{
		"input": "blob:",
		"base": "about:blank",
		"href": "blob:",
		"origin": "null",
		"protocol": "blob:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "",
		"search": "",
		"hash": ""
	},
	"Invalid IPv4 radix digits",
	{
		"input": "http://0x7f.0.0.0x7g",
		"base": "about:blank",
		"href": "http://0x7f.0.0.0x7g/",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "0x7f.0.0.0x7g",
		"hostname": "0x7f.0.0.0x7g",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	{
		"input": "http://0X7F.0.0.0X7G",
		"base": "about:blank",
		"href": "http://0x7f.0.0.0x7g/",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "0x7f.0.0.0x7g",
		"hostname": "0x7f.0.0.0x7g",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	"Invalid IPv4 portion of IPv6 address",
	{
		"input": "http://[::127.0.0.0.1]",
		"base": "about:blank",
		"failure": true
	},
	"Uncompressed IPv6 addresses with 0",
	{
		"input": "http://[0:1:0:1:0:1:0:1]",
		"base": "about:blank",
		"href": "http://[0:1:0:1:0:1:0:1]/",
		"protocol": "http:",
		"username": "",
		"password": "",
		"host": "[0:1:0:1:0:1:0:1]",
		"hostname": "[0:1:0:1:0:1:0:1]",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	}
];

runTests(tests);
