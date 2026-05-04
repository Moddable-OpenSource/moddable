export default function(test) {
	let result;
	
	result = test.add32_t(1111_1111, 2222_2222)
	trace(`add32_t ${ result }\n`);
	
	result = test.add64_t(1111_1111_1111_1111n, 2222_2222_2222_2222n)
	trace(`add64_t ${ result }\n`);

	const date = new Date();
	result = test.nameDay(date.getDay());
	trace(`nameDay ${ result }\n`);
	
	result = test.catenate("a", "bc");
	trace(`catenate ${ result }\n`);

	result = new Uint8Array([1, 2, 3, 4, 5, 6, 7, 8, 9]);
	result = test.sumBytes(result.buffer, result.byteOffset, result.byteLength);
	trace(`sumBytes ${ result }\n`);

	result = new Uint8Array(new ArrayBuffer(10), 3, 5);
	test.fillRandom(result.buffer, result.byteOffset, result.byteLength);
	trace(`fillRandom ${ result }\n`);

	result = new Uint32Array([0, 1, 2]);
	result = test.newTriple(result.buffer, 3);
	trace(`newTriple ${ new Uint32Array(result) }\n`);
	
	try {
		test.newTriple(new ArrayBuffer(3));
	}
	catch (e) {
		trace(`newTriple ${ e }\n`);
	}

	const add32_t = test.add32_t;
	test.add32_t = function(a, b = 1) {
		return add32_t(a, b);
	}
	result = test.add32_t(4)
	trace(`add32_t ${ result }\n`);
	
	const fillRandom = test.fillRandom;
	test.fillRandom = function(buffer, offset, length) {
		if (buffer.byteLength < offset + length)
			throw new RangeError("invalid buffer");
		return fillRandom(buffer, offset, length);
	}
	try {
		test.fillRandom(new ArrayBuffer(0), 0, 1);
	}
	catch (e) {
		trace(`fillRandom ${ e }\n`);
	}
	
	const view = new DataView(new ArrayBuffer(16));
	test.sampleABCSensor(view.buffer);
	trace(`sampleABCSensor ${ view.getInt32(0, 1, true) }, ${ view.getInt32(4, 1, true) }, ${ view.getFloat64(8, 1, true) }\n`);
	
	class ABCSensor {
		#view;
		constructor() {
			this.#view = new DataView(new ArrayBuffer(16));
		}
		sample() {
			const view = this.#view;
			test.sampleABCSensor(view.buffer);
			return {
				a: view.getInt32(0, 1, true),
				b: view.getInt32(4, 1, true),
				c: view.getFloat64(8, 1, true),
			};
		}
	}
	const abcSensor = new ABCSensor();
	result = abcSensor.sample();
	trace(`sample ${ result.a }, ${ result.b }, ${ result.c }\n`);
}
