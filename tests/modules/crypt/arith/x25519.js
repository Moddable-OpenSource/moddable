/*---
description: X25519 against the RFC 7748 vectors
flags: [module]
---*/

import x25519 from "x25519";

const hex = s => Uint8Array.fromHex(s);
const toHex = b => new Uint8Array(b).toHex();

// RFC 7748 section 5.2
assert.sameValue(toHex(x25519(hex("a546e36bf0527c9d3b16154b82465edd62144c0ac1fc5a18506a2244ba449ac4"), hex("e6db6867583030db3594c1a424b15f7c726624ec26b3353b10a903a6d0ab1c4c"))),
	"c3da55379de9c6908e94ea4df28d084f32eccf03491c71f754b4075577a28552", "vector 1");
assert.sameValue(toHex(x25519(hex("4b66e9d4d1b4673c5ad22691957d6af5c11b6421e0ea01d42ca4169e7918ba0d"), hex("e5210f12786811d3f4b7959d0538ae2c31dbe7106fc03c3efc4cd549c715a493"))),
	"95cbde9476e8907d7aade45cb4b873f88b595a68799fa152e6f8f7647aac7957", "vector 2");

// RFC 7748 section 6.1: Diffie-Hellman with the base point 9
const nine = new Uint8Array(32); nine[0] = 9;
const alice = hex("77076d0a7318a57d3c16c17251b26645df4c2f87ebc0992ab177fba51db92c2a");
const bob = hex("5dab087e624a8a4b79e17f8b83800ee66f3bb1292618b6fd1c2f8b27ff88e0eb");
const alicePublic = x25519(alice, nine), bobPublic = x25519(bob, nine);
assert.sameValue(toHex(alicePublic), "8520f0098930a754748b7ddcb43ef75a0dbf3a0d26381af4eba4a98eaa9b4e6a", "Alice's public key");
assert.sameValue(toHex(bobPublic), "de9edb7d7b7dc1b4d35b61c2ece435373f8343c85b78674dadfc7e146f882b4f", "Bob's public key");
const shared = "4a5d9d5ba4ce2de1728e3bf480350f25e07e21c947d19e3376f09b3c1e161742";
assert.sameValue(toHex(x25519(alice, bobPublic)), shared, "shared secret, Alice's side");
assert.sameValue(toHex(x25519(bob, alicePublic)), shared, "shared secret, Bob's side");

assert.throws(RangeError, () => x25519(new Uint8Array(31), nine), "scalar length");

// small-order points (RFC 7748 section 6.1): the result is all zero, and must not hang
const zeroU = new Uint8Array(32);
assert.sameValue(toHex(x25519(alice, zeroU)), "0".repeat(64), "u = 0 gives all zero");
const one = new Uint8Array(32); one[0] = 1;
assert.sameValue(toHex(x25519(alice, one)), "0".repeat(64), "u = 1 gives all zero");
