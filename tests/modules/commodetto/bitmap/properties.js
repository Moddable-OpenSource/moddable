/*---
description: 
flags: [module]
---*/

import Bitmap from "commodetto/Bitmap";


const buffer = new ArrayBuffer(144);
const b = new Bitmap(4, 9, Bitmap.Gray256, buffer, 8);

assert.throws(TypeError, () => b.width = 50, "set width");
assert.throws(TypeError, () => b.height = 50, "set height");
assert.throws(TypeError, () => b.pixelFormat = Bitmap.Default, "set pixelFormat");
assert.throws(TypeError, () => b.buffer = new ArrayBuffer(32), "set buffer");
assert.throws(TypeError, () => b.offset = 0, "set offset");

assert.sameValue(4, b.width, "width");
assert.sameValue(9, b.height, "height");
assert.sameValue(Bitmap.Gray256, b.pixelFormat, "pixelFormat");
assert.sameValue(buffer, b.buffer, "buffer");
assert.sameValue(8, b.offset, "offset");

const b2 = new Bitmap(4, 9, Bitmap.Gray256, new SharedArrayBuffer(40), 4);
assert.sameValue(4, b2.offset, "offset");


class TestBitmap extends Bitmap {
	getWidth() {
		return super.width;
	}
	getHeight() {
		return super.height;
	}
	getPixelFormat() {
		return super.pixelFormat;
	}
	getOffset() {
		return super.offset;
	}
}

let t = new TestBitmap(4, 9, Bitmap.Gray256, buffer, 8);

assert.throws(SyntaxError, () => t.getWidth.call(new $TESTMC.HostObjectChunk), "get width invalid this");
assert.throws(SyntaxError, () => t.getHeight.call(new $TESTMC.HostObjectChunk), "get height invalid this");
assert.throws(SyntaxError, () => t.getPixelFormat.call(new $TESTMC.HostObjectChunk), "get pixelFormamt invalid this");
assert.throws(SyntaxError, () => t.getOffset.call(new $TESTMC.HostObjectChunk), "get offset invalid this");

assert.throws(SyntaxError, () => t.getWidth.call(new $TESTMC.HostObject), "get width invalid this");
assert.throws(SyntaxError, () => t.getHeight.call(new $TESTMC.HostObject), "get height invalid this");
assert.throws(SyntaxError, () => t.getPixelFormat.call(new $TESTMC.HostObject), "get pixelFormamt invalid this");
assert.throws(SyntaxError, () => t.getOffset.call(new $TESTMC.HostObject), "get offset invalid this");
