class Storage @ "xs_directorystorage_destructor" {
	constructor() {throw new TypeError}
	close() @ "xs_directorystorage_close"

	delete(key) @ "xs_directorystorage_delete"
	read(key) @  "xs_directorystorage_read"
	write(key, value) @  "xs_directorystorage_write"
	[Symbol.iterator]() {
		return new StorageIterator(this);
	}

	get format() @ "xs_directorystorage_format_get"
	set format() @ "xs_directorystorage_format_set"
}

function open(options, prototype) @ "xs_directorystorage_open"

class StorageIterator @ "xs_storageIterator_destructor" {
	constructor(storage) @ "xs_storageIterator_constructor"
	next() @ "xs_storageIterator_next"
	return() @ "xs_storageIterator_return"
}

export default Object.freeze({
	open(options) {
		return open(options, Storage.prototype);
	}
}, true);
