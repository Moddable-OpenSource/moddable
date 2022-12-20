/*---
description:
flags: [module]
---*/

import { runTests } from "./url_FIXTURE.js";

const tests = [
  "Other types of space (no-break, zero-width, zero-width-no-break) are name-prepped away to nothing. U+200B, U+2060, and U+FEFF, are ignored",
  {
    "input": "http://GOO\u200b\u2060\ufeffgoo.com",
    "base": "http://other.com/",
    "href": "http://googoo.com/",
    "origin": "http://googoo.com",
    "protocol": "http:",
    "username": "",
    "password": "",
    "host": "googoo.com",
    "hostname": "googoo.com",
    "port": "",
    "pathname": "/",
    "search": "",
    "hash": ""
  },
  "U+3000 is mapped to U+0020 (space) which is disallowed",
  {
    "input": "http://GOO\u00a0\u3000goo.com",
    "base": "http://other.com/",
    "failure": true
  },
  "Ideographic full stop (full-width period for Chinese, etc.) should be treated as a dot. U+3002 is mapped to U+002E (dot)",
  {
    "input": "http://www.foo。bar.com",
    "base": "http://other.com/",
    "href": "http://www.foo.bar.com/",
    "origin": "http://www.foo.bar.com",
    "protocol": "http:",
    "username": "",
    "password": "",
    "host": "www.foo.bar.com",
    "hostname": "www.foo.bar.com",
    "port": "",
    "pathname": "/",
    "search": "",
    "hash": ""
  },
  "Invalid unicode characters should fail... U+FDD0 is disallowed; %ef%b7%90 is U+FDD0",
  {
    "input": "http://\ufdd0zyx.com",
    "base": "http://other.com/",
    "failure": true
  },
  "This is the same as previous but escaped",
  {
    "input": "http://%ef%b7%90zyx.com",
    "base": "http://other.com/",
    "failure": true
  },
  "U+FFFD",
  {
    "input": "https://\ufffd",
    "base": "about:blank",
    "failure": true
  },
  {
    "input": "https://%EF%BF%BD",
    "base": "about:blank",
    "failure": true
  },
  "Domain is ASCII, but a label is invalid IDNA",
  {
    "input": "http://a.b.c.xn--pokxncvks",
    "base": "about:blank",
    "failure": true
  },
  {
    "input": "http://10.0.0.xn--pokxncvks",
    "base": "about:blank",
    "failure": true
  },
  "IDNA labels should be matched case-insensitively",
  {
    "input": "http://a.b.c.XN--pokxncvks",
    "base": "about:blank",
    "failure": true
  },
  {
    "input": "http://a.b.c.Xn--pokxncvks",
    "base": "about:blank",
    "failure": true
  },
  {
    "input": "http://10.0.0.XN--pokxncvks",
    "base": "about:blank",
    "failure": true
  },
  {
    "input": "http://10.0.0.xN--pokxncvks",
    "base": "about:blank",
    "failure": true
  },
  "Test name prepping, fullwidth input should be converted to ASCII and NOT IDN-ized. This is 'Go' in fullwidth UTF-8/UTF-16.",
  {
    "input": "http://Ｇｏ.com",
    "base": "http://other.com/",
    "href": "http://go.com/",
    "origin": "http://go.com",
    "protocol": "http:",
    "username": "",
    "password": "",
    "host": "go.com",
    "hostname": "go.com",
    "port": "",
    "pathname": "/",
    "search": "",
    "hash": ""
  },
  "Basic IDN support, UTF-8 and UTF-16 input should be converted to IDN",
  {
    "input": "http://你好你好",
    "base": "http://other.com/",
    "href": "http://xn--6qqa088eba/",
    "origin": "http://xn--6qqa088eba",
    "protocol": "http:",
    "username": "",
    "password": "",
    "host": "xn--6qqa088eba",
    "hostname": "xn--6qqa088eba",
    "port": "",
    "pathname": "/",
    "search": "",
    "hash": ""
  },
  {
    "input": "https://faß.ExAmPlE/",
    "base": "about:blank",
    "href": "https://xn--fa-hia.example/",
    "origin": "https://xn--fa-hia.example",
    "protocol": "https:",
    "username": "",
    "password": "",
    "host": "xn--fa-hia.example",
    "hostname": "xn--fa-hia.example",
    "port": "",
    "pathname": "/",
    "search": "",
    "hash": ""
  },
  "Fullwidth and escaped UTF-8 fullwidth should still be treated as IP",
  {
    "input": "http://０Ｘｃ０．０２５０．０１",
    "base": "http://other.com/",
    "href": "http://192.168.0.1/",
    "origin": "http://192.168.0.1",
    "protocol": "http:",
    "username": "",
    "password": "",
    "host": "192.168.0.1",
    "hostname": "192.168.0.1",
    "port": "",
    "pathname": "/",
    "search": "",
    "hash": ""
  },
   "URL spec forbids the following. https://www.w3.org/Bugs/Public/show_bug.cgi?id=24257",
  {
    "input": "http://％４１.com",
    "base": "http://other.com/",
    "failure": true
  },
  {
    "input": "http://%ef%bc%85%ef%bc%94%ef%bc%91.com",
    "base": "http://other.com/",
    "failure": true
  },
  "...%00 in fullwidth should fail (also as escaped UTF-8 input)",
  {
    "input": "http://％００.com",
    "base": "http://other.com/",
    "failure": true
  },
  {
    "input": "http://%ef%bc%85%ef%bc%90%ef%bc%90.com",
    "base": "http://other.com/",
    "failure": true
  },
  {
    "input": "ftp://%e2%98%83",
    "base": "about:blank",
    "href": "ftp://xn--n3h/",
    "origin": "ftp://xn--n3h",
    "protocol": "ftp:",
    "username": "",
    "password": "",
    "host": "xn--n3h",
    "hostname": "xn--n3h",
    "port": "",
    "pathname": "/",
    "search": "",
    "hash": ""
  },
  {
    "input": "https://%e2%98%83",
    "base": "about:blank",
    "href": "https://xn--n3h/",
    "origin": "https://xn--n3h",
    "protocol": "https:",
    "username": "",
    "password": "",
    "host": "xn--n3h",
    "hostname": "xn--n3h",
    "port": "",
    "pathname": "/",
    "search": "",
    "hash": ""
  },
  "IDNA ignored code points in file URLs hosts",
  {
    "input": "file://a\u00ADb/p",
    "base": "about:blank",
    "href": "file://ab/p",
    "protocol": "file:",
    "username": "",
    "password": "",
    "host": "ab",
    "hostname": "ab",
    "port": "",
    "pathname": "/p",
    "search": "",
    "hash": ""
  },
  {
    "input": "file://a%C2%ADb/p",
    "base": "about:blank",
    "href": "file://ab/p",
    "protocol": "file:",
    "username": "",
    "password": "",
    "host": "ab",
    "hostname": "ab",
    "port": "",
    "pathname": "/p",
    "search": "",
    "hash": ""
  },
  "Empty host after the domain to ASCII",
  {
    "input": "file://\u00ad/p",
    "base": "about:blank",
    "failure": true
  },
  {
    "input": "file://%C2%AD/p",
    "base": "about:blank",
    "failure": true
  },
  {
    "input": "file://xn--/p",
    "base": "about:blank",
    "failure": true
  },
  {
    "input": "http://💩.123/",
    "base": "about:blank",
    "failure": true
  },
	{
		"input": "blob:https://example.com:443/",
		"base": "about:blank",
		"href": "blob:https://example.com:443/",
		"origin": "https://example.com",
		"protocol": "blob:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "https://example.com:443/",
		"search": "",
		"hash": ""
	},
];


// function printTest(test) {
// 	try {
// 		const url = new URL(test.input, test.base);
// 		if (test.failure) {
//   			print("Expected failure but got «" + url.href + "»");
//   			return;
// 		}
// 		if (test.href != url.href)
// 	  		print("Expected «" + test.href + "» but got «" + url.href + "»");
// 	}
// 	catch(e) {
// 		if (!test.failure) {
// 			print("Expected «" + test.href + "» but got «" + e.message + "»");
//   			return;
// 		}
// 		if (e.constructor == SyntaxError) {
// 			print("Expected «TypeError» but got «" + e.message + "»");
//   			return;
// 		}
// 	}
// }
// 
// function printTests(tests) {
// 	for (let test of tests) {
// 		if (typeof(test) == "object")
// 			printTest(test);
// 	}
// }

runTests(tests);
