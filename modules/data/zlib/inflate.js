class Inflate @ "xs_inflate_destructor" {
	chunks = [];
	strm = {};

	constructor(options = {}) {
		this.build(options);
	}
	build(options) @ "xs_inflate";
	close() @ "xs_inflate_close";

	push(buffer, end) {
		this.strm.avail_in = this._push(buffer, end);
	}

	_push(buffer, end) @ "xs_inflate_push";

	onData(chunk) {
		this.chunks.push(chunk);
	}
	onEnd(err) {
		const chunks = this.chunks, length = chunks.length;
		delete this.chunks;

		this.err = err;
		if (err)
			return;

		// join chunks
		let total = 0;
		for (let i = 0; i < length; i++)
			total += chunks[i].byteLength;

		this.result = new Uint8Array(total);
		let offset = 0;
		for (let i = 0; i < length; i++) {
			const chunk = chunks[i];
			this.result.set(new Uint8Array(chunk), offset);
			offset += chunk.byteLength;
		}
	}
}

export default Inflate;
