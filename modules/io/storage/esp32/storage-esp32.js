class Storage extends Native("xs_directorystorage_destructor") {
	constructor() {throw new TypeError}
	close() { return native("xs_directorystorage_close").call(this); }

	delete(key) { return native("xs_directorystorage_delete").call(this, key); }
	read(key) { return native("xs_directorystorage_read").call(this, key); }
	write(key, value) { return native("xs_directorystorage_write").call(this, key, value); }
	[Symbol.iterator]() {
		return new StorageIterator(this);
	}

	get format() { return native("xs_directorystorage_format_get").call(this); }
	set format(value) { native("xs_directorystorage_format_set").call(this, value); }
}

function open(options, prototype) { return native("xs_directorystorage_open").call(this, options, prototype); }

class StorageIterator extends Native("xs_storageIterator_destructor") {
	constructor(storage) { super(); native("xs_storageIterator_constructor").call(this, storage); }
	next() { return native("xs_storageIterator_next").call(this); }
	return() { return native("xs_storageIterator_return").call(this); }
}

export default Object.freeze({
	open(options) {
		return open(options, Storage.prototype);
	}
}, true);
