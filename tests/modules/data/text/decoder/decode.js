/*---
description: 
flags: [module]
---*/

import TextDecoder from "text/decoder";

const decoder = new TextDecoder;
const decoderFatal = new TextDecoder("utf-8", {fatal: true});
const decoderIgnoreBOM = new TextDecoder("utf-8", {ignoreBOM: true});

assert.throws(SyntaxError, () => decoder.decode.call(new $TESTMC.HostObject), "decode with invalid this");
assert.throws(SyntaxError, () => decoder.decode.call(new $TESTMC.HostObjectChunk), "decode with invalid this");

assert.sameValue("A", decoder.decode(Uint8Array.of(65)));
assert.sameValue("AB", decoder.decode(Uint8Array.of(65, 66)));
assert.sameValue("A\0B", decoder.decode(Uint8Array.of(65, 0, 66)));
assert.sameValue("©!", decoder.decode(Uint8Array.of(0xC2, 0xA9, 33)));
assert.throws(TypeError, () => decoderFatal.decode(Uint8Array.of(65, 0xA9, 33)));

assert.sameValue("!\uFFFD!", decoder.decode(Uint8Array.of(33, 0xC2, 33)));
assert.sameValue("!\uFFFD\uFFFD!", decoder.decode(Uint8Array.of(33, 0xC2, 0xC2, 33)));
assert.sameValue("!\uFFFD\uFFFD", decoder.decode(Uint8Array.of(33, 0xC2, 0xC2)));
assert.sameValue("!\uFFFD\uFFFD\uFFFD!", decoder.decode(Uint8Array.of(33, 0xC2, 0xC2, 0xEF, 0xBF, 33)));

// sequence from https://bugzilla.mozilla.org/show_bug.cgi?id=746900 (results from browsers, circa 2021)
assert.sameValue("\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD", decoder.decode(Uint8Array.of(0xF8, 0x80, 0x80, 0x80, 0x80, 0x80)));
assert.sameValue("\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD", decoder.decode(Uint8Array.of(0xF8, 0x80, 0x80, 0x80, 0x80)));
assert.sameValue("\uFFFD\uFFFD\uFFFDA", decoder.decode(Uint8Array.of(0xF0, 0x80, 0x80, 0x41)));
assert.sameValue("\uFFFD\uFFFDB", decoder.decode(Uint8Array.of(0xF0, 0x80, 0x42)));
assert.sameValue("\uFFFDC", decoder.decode(Uint8Array.of(0xF0, 0x43)));

assert.sameValue("A", decoder.decode(Uint8Array.of(65, 0xC2), {stream: true}));
assert.sameValue("©!", decoder.decode(Uint8Array.of(0xA9, 33)));

assert.sameValue("A", decoder.decode(Uint8Array.of(65, 0xC2), {stream: true}));
assert.sameValue("©!", decoder.decode(Uint8Array.of(0xA9, 33), {stream: true}));

assert.sameValue("", decoderFatal.decode(Uint8Array.of(0xC2), {stream: true}));
assert.throws(TypeError, () => decoderFatal.decode(Uint8Array.of(65)));

assert.sameValue("A", decoder.decode(Uint8Array.of(0xEF, 0xBB, 0xBF, 65)));
assert.sameValue("\uFEFFA", decoderIgnoreBOM.decode(Uint8Array.of(0xEF, 0xBB, 0xBF, 65)));
