class Flash @ "xs_flashstorage_destructor" {
	constructor() {throw new TypeError}
	close() @ "xs_flashstorage_close"

	eraseBlock(start, end) @ "xs_flashstorage_eraseBlock"

	read(byteLength, byteOffset) @  "xs_flashstorage_read"
	write(buffer, byteOffset) @  "xs_flashstorage_write"

	status() @ "xs_flashstorage_status"

	get format() {
		return "buffer";
	}
	set format(value) {
		if (value != "buffer")
			throw new RangeError("invalid");
	}
}

function open(options, constructor) @ "xs_flashstorage_open"

class FlashPartitionIterator @ "xs_flashIterator_destructor" {
	constructor() @ "xs_flashIterator_"
	next() @ "xs_flashIterator_next"
	return() @ "xs_flashIterator_return"
}

export default Object.freeze({
	open(options) {
		return open(options, Flash.prototype);		
	},
	[Symbol.iterator]() {
		return new FlashPartitionIterator;
	}
}, true);
