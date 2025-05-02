import config from "mc/config";
class FlashPartition @ "xs_flash_partition_destructor" {
	constructor() { throw new TypeError("use device.flash.open({ path })"); }
	close() @ "xs_flash_partition_close"

	eraseBlock(start, end) @ "xs_flash_partition_eraseBlock"
	read(buffer, byteOffset) @  "xs_flash_partition_read"
	status() @ "xs_flash_partition_status"
	write(buffer, byteOffset) @  "xs_flash_partition_write"

	get format() @ "xs_flash_partition_get_format"
	set format() @ "xs_flash_partition_set_format"
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

function openPartition(options, prototype) @ "xs_flash_partition_open"

export default {
	open(options) {
		return openPartition(options, FlashPartition.prototype, config.flash);
	},
	[Symbol.iterator]() {
		return new FlashPartitionIterator(config.flash);
	}
};
