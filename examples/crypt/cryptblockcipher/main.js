/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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
import Hex from "hex";

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

function testCipherMode(name, direction, cipher, iv, padding, data, expected)
{
	let mode = new Mode(name, cipher, iv, padding);

	let result = mode[direction](data);

	result = Hex.toString(result);
	expected = Hex.toString(expected);

	if (result != expected)
		trace(`${name}.${direction} FAIL\n`);
	else
		trace(`${name}.${direction} pass\n`);
}

function testBlockCipher(name, keyStr, plain, expected)
{
	let key = ("string" == typeof keyStr) ? ArrayBuffer.fromString(keyStr) : keyStr;

	let result = (new BlockCipher(name, key)).encrypt(plain + "");

	if (Hex.toString(result) != expected)
		trace(`${name} FAIL\n`);
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
