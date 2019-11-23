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

import {BlockCipher, StreamCipher, Mode} from "crypt";
import GCM from "gcm";
import Bin from "bin";

let Hex = {
	toBuffer(s) {
		return ArrayBuffer.fromBigInt(BigInt("0x" + s), s.length / 2);
	},
	toString(b) {
		return BigInt.fromArrayBuffer(b).toString(16);
	},
};

testCipherMode("ECB", "decrypt", new BlockCipher("DES", ArrayBuffer.fromString("ABCDEFGH")), undefined, undefined, Hex.toBuffer("5BD7DE165BB69D60D04399DDAD80227B"), ArrayBuffer.fromString("0123456789abcdef"));
testCipherMode("ECB", "encrypt", new BlockCipher("DES", ArrayBuffer.fromString("ABCDEFGH")), undefined, undefined, ArrayBuffer.fromString("0123456789abcdef"), Hex.toBuffer("5BD7DE165BB69D60D04399DDAD80227B"));


testCipherMode("CBC", "decrypt", new BlockCipher("DES", ArrayBuffer.fromString("ABCDEFGH")), undefined, undefined, Hex.toBuffer("77DEDD68ED79D710CA4E42C674850E61"), ArrayBuffer.fromString("this is a test!!"));
testCipherMode("CBC", "encrypt", new BlockCipher("DES", ArrayBuffer.fromString("ABCDEFGH")), undefined, undefined, ArrayBuffer.fromString("this is a test!!"), Hex.toBuffer("77DEDD68ED79D710CA4E42C674850E61"));

testCipherMode("CTR", "decrypt", new BlockCipher("AES", Hex.toBuffer("AE6852F8121067CC4BF7A5765577F39E")),
				Hex.toBuffer("00000030000000000000000000000001"), undefined, Hex.toBuffer("E4095D4FB7A7B3792D6175A3261311B8"), ArrayBuffer.fromString("Single block msg"));

testCipherMode("CTR", "decrypt", new BlockCipher("AES", Hex.toBuffer("7E24067817FAE0D743D6CE1F32539163")),
				Hex.toBuffer("006CB6DBC0543B59DA48D90B00000001"), undefined,
				Hex.toBuffer("5104A106168A72D9790D41EE8EDAD388EB2E1EFC46DA57C8FCE630DF9141BE28"),
				Hex.toBuffer("000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F"));

testCipherMode("CTR", "encrypt", new BlockCipher("AES", Hex.toBuffer("7E24067817FAE0D743D6CE1F32539163")),
				Hex.toBuffer("006CB6DBC0543B59DA48D90B00000001"), undefined,
				Hex.toBuffer("000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F"),
				Hex.toBuffer("5104A106168A72D9790D41EE8EDAD388EB2E1EFC46DA57C8FCE630DF9141BE28"));

testBlockCipher("DES", "ABCDEFGH", "01234567", "5BD7DE165BB69D60");
testBlockCipher("TDES", "ABCDEFGHIJKLMNOPQRSTUVWX", "01234567", "4F598233C4E86579");
testBlockCipher("AES", "ABCDEFGHIJKLMNOP", "0123456789ABCDEF", "5AD42F600746B101FA1C7C2318542CD6");

testStreamCipher("RC4", "ABCDEFGH", "01234567", undefined, 0, "EFB6FFCB07AEB5A3");
testStreamCipher("ChaCha", new ArrayBuffer(32), new ArrayBuffer(64), new ArrayBuffer(12), 0, "76B8E0ADA0F13D90405D6AE55386BD28BDD219B8A08DED1AA836EFCC8B770DC7DA41597C5157488D7724E03FB8D84A376A43B8F41518A11CC387B669B2EE6586");

testCipherMode("GCM", "encrypt", new BlockCipher("AES", Hex.toBuffer("00000000000000000000000000000000")), Hex.toBuffer("000000000000000000000000"), undefined, new ArrayBuffer(), Hex.toBuffer("58e2fccefa7e3061367f1d57a4e7455a"));
// testCipherMode("GCM", "decrypt", new BlockCipher("AES", Hex.toBuffer("00000000000000000000000000000000")), Hex.toBuffer("000000000000000000000000"), undefined, Hex.toBuffer("58e2fccefa7e3061367f1d57a4e7455a"), new ArrayBuffer());	// ArrayBuffer can't be empty??

let cipher = new BlockCipher("AES", Hex.toBuffer("feffe9928665731c6d6a8f9467308308"));
let iv = Hex.toBuffer("cafebabefacedbaddecaf888");
let aad = Hex.toBuffer("feedfacedeadbeeffeedfacedeadbeefabaddad2");
let tag = Hex.toBuffer("5bc94fbc3221a5db94fae95ae7121a47");
let plain = Hex.toBuffer("d9313225f88406e5a55909c5aff5269a86a7a9531534f7da2e4c303d8a318a721c3c0c95956809532fcf0e2449a6b525b16aedf5aa0de657ba637b39");
let cipherText = Hex.toBuffer("42831ec2217774244b7221b784d0d49ce3aa212f2c02a4e035c17e2329aca12e21d514b25466931c7d8f6a5aac84aa051ba30b396a0aac973d58e091");
testCipherMode("GCM", "encrypt", cipher, iv, aad, plain, cipherText.concat(tag));
testCipherMode("GCM", "decrypt", cipher, iv, aad, cipherText.concat(tag), plain);



function testCipherMode(name, direction, cipher, iv, padding, data, expected)
{
	let mode, result;
	if (name == "GCM") {
		mode = new GCM(cipher);
		result = mode.process(data, undefined, iv, padding /* AAD */, direction == "encrypt");
	}
	else {
		mode = new Mode(name, cipher, iv, padding);
		result = mode[direction](data);
	}

	if (Bin.comp(result, expected) != 0) {
		trace(`${name}.${direction} FAIL\n`);
		trace("result = " + Hex.toString(result) + "\n");
		trace("expected = " + Hex.toString(expected) + "\n");
	}
	else
		trace(`${name}.${direction} pass\n`);
}

function testBlockCipher(name, keyStr, plain, expected)
{
	let key = ("string" == typeof keyStr) ? ArrayBuffer.fromString(keyStr) : keyStr;
	expected = Hex.toBuffer(expected);

	let result = (new BlockCipher(name, key)).encrypt(plain + "");

	if (Bin.comp(result, expected) != 0)
		trace(`${name} FAIL\n`);
	else
		trace(`${name} pass\n`);

	plain = ArrayBuffer.fromString(plain);
	result = (new BlockCipher(name, key)).decrypt(result)
	
	if (Bin.comp(result, plain) != 0) {
		trace(`${name} FAIL\n`);
		trace("result = " + Hex.toString(result) + "\n");
		trace("plain = " + Hex.toString(plain) + "\n");
	}
	else
		trace(`${name} pass\n`);
}

function testStreamCipher(name, keyStr, plain, iv, counter, expected)
{
	let key = ("string" == typeof keyStr) ? ArrayBuffer.fromString(keyStr) : keyStr;

	if ("string" == typeof plain)
		plain += "";

	let result = (new StreamCipher(name, key, iv, counter)).encrypt(plain);

	if (Hex.toString(result) != expected)
		trace(`${name} fail\n`);
	else
		trace(`${name} pass\n`);
}
