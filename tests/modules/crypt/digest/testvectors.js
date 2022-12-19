/*---
description: 
flags: [module]
---*/

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
assert.sameValue("s3pPLMBiTxaQ9kYGzzhZRbK+xOo=", result, "SHA1");

let ghash = new GHASH(H2B("0x66e94bd4ef8a2c3b884cfa59ca342b2e"));
ghash.write(H2B("0x0388dace60b6a392f328c2b971b2fe78"));
result = ghash.close();
let expected = H2B("0xf38cbb1ad69223dcc3457ae5b6b0f885");
assert(Bin.comp(result, expected) == 0, "GHASH 1");

ghash = new GHASH(H2B("0xb83b533708bf535d0aa6e52980d53b78"),
		  H2B("0xfeedfacedeadbeeffeedfacedeadbeefabaddad2"));
ghash.write(H2B("0x42831ec2217774244b7221b784d0d49ce3aa212f2c02a4e035c17e2329aca12e21d514b25466931c7d8f6a5aac84aa051ba30b396a0aac973d58e091"));
result = ghash.close();
expected = H2B("0x698e57f70e6ecc7fd9463b7260a9ae5f");
assert(Bin.comp(result, expected) == 0, "GHASH 2");
