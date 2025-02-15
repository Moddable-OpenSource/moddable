import bootstrap from "embedded:storage/key-value"

const storage = globalThis.device?.storage ?? bootstrap;

function keys(store) {
	return Array.from(store).sort();
}

function emptyDomain(store) {
	const k = keys(store)
	for (let key of k)
		store.delete(key);
	
	assert.sameValue(Array.from(store).length, 0, "emptyDomain failed");
}

export {storage as default, emptyDomain, keys}
