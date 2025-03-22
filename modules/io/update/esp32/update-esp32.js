class Update @ "xs_update_destructor" {
	constructor() {throw new TypeError}
	close() @ "xs_update_close"
	complete() @ "xs_update_complete"

	write(buffer, byteOffset) @  "xs_update_write"

	get format() {
		return "buffer";
	}
	set format(value) {
		if (value != "buffer")
			throw new RangeError("invalid");
	}
}

function open(options, prototype) @ "xs_update_open"

export default Object.freeze({
	open(options) {
		return open(options, Update.prototype);		
	},
}, true);
