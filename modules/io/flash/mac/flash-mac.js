import config from "mc/config";
class FlashPartition extends Native("xs_flash_partition_destructor") {
	constructor() { throw new TypeError("use device.flash.open({ path })"); }
	close() { return native("xs_flash_partition_close").call(this); }

	eraseBlock(start, end) { return native("xs_flash_partition_eraseBlock").call(this, start, end); }
	read(buffer, byteOffset) { return native("xs_flash_partition_read").call(this, buffer, byteOffset); }
	status() { return native("xs_flash_partition_status").call(this); }
	write(buffer, byteOffset) { return native("xs_flash_partition_write").call(this, buffer, byteOffset); }

	get format() { return native("xs_flash_partition_get_format").call(this); }
	set format(value) { native("xs_flash_partition_set_format").call(this, value); }
}

class FlashPartitionIterator {
	constructor() {
		const flash = config.flash;
		const keys = [];
		for (let key in flash)
			keys.push(key);
		this.iterator = keys.values();
	}
	next() {
		return this.iterator.next();
	}
}

function openPartition(options, prototype) { return native("xs_flash_partition_open").call(this, options, prototype); }

export default {
	open(options) {
		return openPartition(options, FlashPartition.prototype, config.flash);
	},
	[Symbol.iterator]() {
		return new FlashPartitionIterator(config.flash);
	}
};
