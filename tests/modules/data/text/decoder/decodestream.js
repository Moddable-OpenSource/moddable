/*---
description: 
flags: [module]
---*/

import TextDecoder from "text/decoder";

let decoder = new TextDecoder;

// break in two byte sequence (C4 A2)
assert.sameValue("", decoder.decode(Uint8Array.of(0xC4), {stream: true}));
assert.sameValue("Ģ", decoder.decode(Uint8Array.of(0xA2), {stream: true}));

assert.sameValue("", decoder.decode(Uint8Array.of(0xC4), {stream: true}));
assert.sameValue("Ģ", decoder.decode(Uint8Array.of(0xA2)));

assert.sameValue("", decoder.decode(Uint8Array.of(0xC4), {stream: true}));
assert.sameValue("\uFFFD", decoder.decode());

// break in three byte sequence (E9 A1 B9)
assert.sameValue("", decoder.decode(Uint8Array.of(0xE9), {stream: true}));
assert.sameValue("", decoder.decode(Uint8Array.of(0xA1), {stream: true}));
assert.sameValue("项", decoder.decode(Uint8Array.of(0xB9), {stream: true}));

assert.sameValue("", decoder.decode(Uint8Array.of(0xE9), {stream: true}));
assert.sameValue("", decoder.decode(Uint8Array.of(0xA1), {stream: true}));
assert.sameValue("项", decoder.decode(Uint8Array.of(0xB9)));

assert.sameValue("", decoder.decode(Uint8Array.of(0xE9, 0xA1), {stream: true}));
assert.sameValue("项", decoder.decode(Uint8Array.of(0xB9), {stream: true}));

assert.sameValue("", decoder.decode(Uint8Array.of(0xE9, 0xA1), {stream: true}));
assert.sameValue("项", decoder.decode(Uint8Array.of(0xB9)));

assert.sameValue("", decoder.decode(Uint8Array.of(0xE9), {stream: true}));
assert.sameValue("项", decoder.decode(Uint8Array.of(0xA1, 0xB9), {stream: true}));

assert.sameValue("", decoder.decode(Uint8Array.of(0xE9), {stream: true}));
assert.sameValue("项", decoder.decode(Uint8Array.of(0xA1, 0xB9)));

assert.sameValue("", decoder.decode(Uint8Array.of(0xE9), {stream: true}));
assert.sameValue("\uFFFD", decoder.decode());

assert.sameValue("", decoder.decode(Uint8Array.of(0xE9, 0xA1), {stream: true}));
assert.sameValue("\uFFFD", decoder.decode());

assert.sameValue("", decoder.decode(Uint8Array.of(0xF0, 0x9F, 0x92), {stream: true}));
assert.sameValue("\uFFFD", decoder.decode());

// illegal sequence spanning a buffered partial lead and a short final chunk:
// the recovery scan must consume from the buffered bytes before src and stop at the
// end of the input. The chunk is a view whose backing store holds continuation bytes
// (0x90) past its length, so the bytes seen are F0 80 90 90: the 0x80 is below the
// 0x90 lower bound for an F0 lead, so it and the two trailing 0x90 each decode to U+FFFD.
assert.sameValue("", decoder.decode(Uint8Array.of(0xF0, 0x80), {stream: true}));
assert.sameValue("\uFFFD\uFFFD\uFFFD\uFFFD", decoder.decode(new Uint8Array(8).fill(0x90).subarray(0, 2), {stream: true}));
assert.sameValue("", decoder.decode());
