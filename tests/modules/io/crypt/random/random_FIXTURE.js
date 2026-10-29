import Random from "embedded:crypt/random";
export default Random;

// a run of bytes from a cryptographic source: every value in 0..255 should appear in 8 KB (expected miss rate ~1e-14)
export function looksRandom(bytes) {
	const seen = new Uint8Array(256);
	for (const b of bytes)
		seen[b] = 1;
	return seen.every(v => v);
}
