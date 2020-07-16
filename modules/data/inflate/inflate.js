class Inflate @ "xs_inflate_destructor" {
	constructor(options = {}) {
		this.chunks = [];
		this.strm = {};

		this.build(options);
	}
	build(options) @ "xs_inflate";
	close() @ "xs_inflate_close";

	push(buffer, end) {
		if (ArrayBuffer.isView(buffer)) {
			if (buffer.byteOffset || (buffer.byteLength !== buffer.buffer.byteLength))
				throw new Error;

			buffer = buffer.buffer;
		}
		this.strm.avail_in = this._push(buffer, end);
	}

	_push(buffer, end) @ "xs_inflate_push";

	onData(chunk) {
		this.chunks.push(chunk);
	}
	onEnd(err) {
		this.err = err;
		if (err) {
			delete this.chunks;
			return;
		}

		// join chunks
		let total = 0;
		for (let i = 0, chunks = this.chunks, length = chunks.length; i < length; i++)
			total += chunks[i].byteLength;

		this.result = new Uint8Array(total);
		let offset = 0;
		for (let i = 0, chunks = this.chunks, length = chunks.length; i < length; i++) {
			this.result.set(new Uint8Array(this.chunks[i]), offset);
			offset += chunks[i].byteLength;
		}
		delete this.chunks;
	}
}

export default Inflate;
