export class BitsView extends DataView {
//	getIntBits(bitsOffset, bitsSize, endian) @ "BitsView_prototype_getIntBits"
	getUintBits(bitsOffset, bitsSize, endian) @ "BitsView_prototype_getUintBits"
//	setIntBits(bitsOffset, bitsSize, value, endian) @ "BitsView_prototype_setIntBits"
	setUintBits(bitsOffset, bitsSize, value, endian) @ "BitsView_prototype_setUintBits"
}

export class UintBitsArray {
	constructor(buffer, bitsSize) {
		if ("number" === typeof buffer) {
			this.length = buffer;
			buffer = new ArrayBuffer(((bitsSize * buffer) + 7) >> 3);
		}
		else if (buffer instanceof ArrayBuffer)
			this.length = (buffer.byteLength / bitsSize) | 0;
		else
			throw new Error("invalid buffer");
		this.bitsSize = bitsSize;
		return new Proxy(new BitsView(buffer), this);
	}
	get(target, key) {
		if ("length" === key)
			return this.length;
		if ("buffer" === key)
			return target.buffer;
		const bitsSize = this.bitsSize;
		return target.getUintBits(key * bitsSize, bitsSize);
	}
	set(target, key, value) {
		const bitsSize = this.bitsSize;
		return target.setUintBits(key * bitsSize, bitsSize, value);
	}
}
