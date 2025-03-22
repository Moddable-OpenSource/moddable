class StorageDomain @ "xs_storage_domain_destructor" {
	constructor() { throw new TypeError("use device.storage.open({ path })"); }
	close() @ "xs_storage_domain_close"

	delete(key) @ "xs_storage_domain_delete"
	read(key) @  "xs_storage_domain_read"
	write(key, value) @  "xs_storage_domain_write"
	[Symbol.iterator]() {
		return new StorageDomainKeyIterator(this);
	}
	
	get format() @ "xs_storage_domain_get_format"
	set format() @ "xs_storage_domain_set_format"
}

function open(options, prototype) @ "xs_storage_domain_open"

class StorageDomainKeyIterator @ "xs_storage_domain_key_iterator_destructor" {
	constructor(storage) @ "xs_storage_domain_key_iterator_constructor"
	next() @ "xs_storage_domain_key_iterator_next"
	return() @ "xs_storage_domain_key_iterator_return"
}

export default Object.freeze({
	open(options) {
		return open(options, StorageDomain.prototype);
	}
}, true);
