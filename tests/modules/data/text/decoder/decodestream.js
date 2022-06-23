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
assert.sameValue("\uFFFD\uFFFD", decoder.decode());
