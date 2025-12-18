class Update extends Native("xs_update_destructor") {
	constructor() {throw new TypeError}
	close() { return native("xs_update_close").call(this); }
	complete() { return native("xs_update_complete").call(this); }

	write(buffer, byteOffset) { return native("xs_update_write").call(this, buffer, byteOffset); }

	get format() {
		return "buffer";
	}
	set format(value) {
		if (value != "buffer")
			throw new RangeError("invalid");
	}
}

function open(options, prototype) { return native("xs_update_open").call(this, options, prototype); }

export default Object.freeze({
	open(options) {
		return open(options, Update.prototype);		
	},
}, true);
