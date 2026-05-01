import FFI from "ffi";

try {
	const test = new FFI;
	let result;
	
	result = test.add32_t(1111_1111, 2222_2222)
	trace(`${ result }\n`);
	
	result = test.add64_t(1111_1111_1111_1111n, 2222_2222_2222_2222n)
	trace(`${ result }\n`);

	const date = new Date();
	result = test.nameDay(date.getDay());
	trace(`${ result }\n`);
	
	result = test.catenate("a", "bc");
	trace(`${ result }\n`);

	result = new Uint8Array([1, 2, 3, 4, 5, 6, 7, 8, 9]);
	result = test.sumBytes(result.buffer, result.byteOffset, result.byteLength);
	trace(`${ result }\n`);

	result = new Uint8Array(new ArrayBuffer(10), 3, 5);
	test.fillRandom(result.buffer, result.byteOffset, result.byteLength);
	trace(`${ result }\n`);

	const add32_t = test.add32_t;
	test.add32_t = function(a, b = 1) {
		return add32_t(a, b);
	}
	result = test.add32_t(4)
	trace(`${ result }\n`);
	
	const view = new DataView(new ArrayBuffer(16));
	test.sampleABCSensor(view.buffer);
	trace(`${ view.getInt32(0, 1, true) }, ${ view.getInt32(4, 1, true) }, ${ view.getFloat64(8, 1, true) }\n`);
	
	class ABCSensor {
		constructor() {
			this.view = new DataView(new ArrayBuffer(16));
			this.result = { a:0, b:0, c:0 };
		}
		sample() {
			const { view, result } = this;
			test.sampleABCSensor(view.buffer);
			result.a = view.getInt32(0, 1, true);
			result.b = view.getInt32(4, 1, true);
			result.c = view.getFloat64(8, 1, true);
			return result;
		}
	}
	const abcSensor = new ABCSensor();
	result = abcSensor.sample();
	trace(`${ result.a }, ${ result.b }, ${ result.c }\n`);
} 
catch(error) {
	console.log("Error on new FFI: " + error);
}
