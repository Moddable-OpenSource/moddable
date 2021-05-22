/*
 * Copyright (c) 2016-2019  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import {Digest, GHASH} from "crypt";
import Base64 from "base64";
import Bin from "bin";

function H2B(hstr)
{
	return ArrayBuffer.fromBigInt(BigInt(hstr));
}

function B2H(b)
{
	return (BigInt.fromArrayBuffer(b)).toString(16);
}

// sample WebSocket handshake hash
let sha1 = new Digest("SHA1");
sha1.write("dGhlIHNhbXBsZSBub25jZQ==");
sha1.write("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
let result = Base64.encode(sha1.close());

trace(`Calculated hash: ${result}\n`);

let expect = "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=";
trace(`Expected hash: ${expect}\n`);

if (result == expect)
	trace("PASS\n");
else
	trace("FAIL\n");

let ghash = new GHASH(H2B("0x66e94bd4ef8a2c3b884cfa59ca342b2e"));
ghash.write(H2B("0x0388dace60b6a392f328c2b971b2fe78"));
result = ghash.close();
let expected = H2B("0xf38cbb1ad69223dcc3457ae5b6b0f885");
if (Bin.comp(result, expected) == 0)
	trace("PASS\n");
else {
	trace("FAIL\n");
	trace("result: " + B2H(result) + "\n");
	trace("expected: " + B2H(expected) + "\n");
	debugger;
}

ghash = new GHASH(H2B("0xb83b533708bf535d0aa6e52980d53b78"),
		  H2B("0xfeedfacedeadbeeffeedfacedeadbeefabaddad2"));
ghash.write(H2B("0x42831ec2217774244b7221b784d0d49ce3aa212f2c02a4e035c17e2329aca12e21d514b25466931c7d8f6a5aac84aa051ba30b396a0aac973d58e091"));
result = ghash.close();
expected = H2B("0x698e57f70e6ecc7fd9463b7260a9ae5f");
if (Bin.comp(result, expected) == 0)
	trace("PASS\n");
else {
	trace("FAIL\n");
	trace("result: " + B2H(result) + "\n");
	trace("expected: " + B2H(expected) + "\n");
	debugger;
}
