class StorageDomain extends Native("xs_storage_domain_destructor") {
	constructor() { throw new TypeError("use device.storage.open({ path })"); }
	close() { return native("xs_storage_domain_close").call(this); }

	delete(key) { return native("xs_storage_domain_delete").call(this, key); }
	read(key) { return native("xs_storage_domain_read").call(this, key); }
	write(key, value) { return native("xs_storage_domain_write").call(this, key, value); }
	[Symbol.iterator]() {
		return new StorageDomainKeyIterator(this);
	}
	
	get format() { return native("xs_storage_domain_get_format").call(this); }
	set format(value) { native("xs_storage_domain_set_format").call(this, value); }
}

function open(options, prototype) { return native("xs_storage_domain_open").call(this, options, prototype); }

class StorageDomainKeyIterator extends Native("xs_storage_domain_key_iterator_destructor") {
	constructor(storage) { super(); native("xs_storage_domain_key_iterator_constructor").call(this, storage); }
	next() { return native("xs_storage_domain_key_iterator_next").call(this); }
	return() { return native("xs_storage_domain_key_iterator_return").call(this); }
}

export default Object.freeze({
	open(options) {
		return open(options, StorageDomain.prototype);
	}
}, true);
