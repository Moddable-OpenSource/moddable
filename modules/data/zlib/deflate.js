class Deflate @ "xs_deflate_destructor" {
	err = 0;
	chunks = [];

	constructor(options = {}) {
		build.call(this, options);
	}
	close() @ "xs_deflate_close";

	push(buffer, end = false) {
		if (ArrayBuffer.isView(buffer)) {
			if (buffer.byteOffset || (buffer.byteLength !== buffer.buffer.byteLength))
				throw new Error;

			buffer = buffer.buffer;
		}
		return push.call(this, buffer, end);
	}
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

function build(options) @ "xs_deflate";
function push(buffer, end) @ "xs_deflate_push";

export default Deflate;
