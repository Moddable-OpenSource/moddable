import Digest from "embedded:crypt/digest";
import TextEncoder from "text/encoder";

export default Digest;

// per-algorithm metadata from the ECMA-419 Crypto Digest spec table
export const algMeta = [
	{ algorithm: "MD5",    blockSize:  64, outputSize: 16 },
	{ algorithm: "SHA-1",   blockSize:  64, outputSize: 20 },
	{ algorithm: "SHA-224", blockSize:  64, outputSize: 28 },
	{ algorithm: "SHA-256", blockSize:  64, outputSize: 32 },
	{ algorithm: "SHA-384", blockSize: 128, outputSize: 48 },
	{ algorithm: "SHA-512", blockSize: 128, outputSize: 64 },
];

// Known-answer vectors. Strings are UTF-8 encoded.
const enc = new TextEncoder();
const ABC = enc.encode("abc");
const EMPTY = new Uint8Array(0);
const QBF = enc.encode("The quick brown fox jumps over the lazy dog");

export const KAT = {
	MD5: [
		{ in: EMPTY, out: "d41d8cd98f00b204e9800998ecf8427e" },
		{ in: ABC,   out: "900150983cd24fb0d6963f7d28e17f72" },
		{ in: QBF,   out: "9e107d9d372bb6826bd81d3542a419d6" },
	],
	"SHA-1": [
		{ in: EMPTY, out: "da39a3ee5e6b4b0d3255bfef95601890afd80709" },
		{ in: ABC,   out: "a9993e364706816aba3e25717850c26c9cd0d89d" },
		{ in: QBF,   out: "2fd4e1c67a2d28fced849ee1bb76e7391b93eb12" },
	],
	"SHA-224": [
		{ in: EMPTY, out: "d14a028c2a3a2bc9476102bb288234c415a2b01f828ea62ac5b3e42f" },
		{ in: ABC,   out: "23097d223405d8228642a477bda255b32aadbce4bda0b3f7e36c9da7" },
	],
	"SHA-256": [
		{ in: EMPTY, out: "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855" },
		{ in: ABC,   out: "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad" },
		{ in: QBF,   out: "d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592" },
	],
	"SHA-384": [
		{ in: EMPTY, out: "38b060a751ac96384cd9327eb1b1e36a21fdb71114be07434c0cc7bf63f6e1da274edebfe76f65fbd51ad2f14898b95b" },
		{ in: ABC,   out: "cb00753f45a35e8bb5a03d699ac65007272c32ab0eded1631a8b605a43ff5bed8086072ba1e7cc2358baeca134c825a7" },
	],
	"SHA-512": [
		{ in: EMPTY, out: "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e" },
		{ in: ABC,   out: "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f" },
	],
};

// Builds a Digest, writes each chunk in order, returns the hex digest, closes.
export function digestOf(options, ...chunks) {
	const d = new Digest(options);
	try {
		for (const c of chunks)
			d.write(c);
		return new Uint8Array(d.read()).toHex();
	}
	finally {
		d.close();
	}
}
