import FFI from "mc/ffi";

try {
	const test = new FFI;
	let result;
	
	result = test.add32_t(1111_1111, 2222_2222)
	trace(`${ result }\n`);
	result = test.add64_t(1111_1111_1111_1111n, 2222_2222_2222_2222n)
	trace(`${ result }\n`);

	const bytes = new Uint8Array(5);
	test.hello("hello", bytes.buffer, bytes.length);
	trace(`bytes = ${ bytes }\n`);
	
	trace(`catenate("5", "6") = ${ test.catenate("5", "6") }\n`);
	
	const date = new Date();
	trace(`aujourd'hui = ${ test.nameDay(date.getDay()) }\n`);
	
	const buffer = new ArrayBuffer(12);
	const view = new DataView(buffer);
	view.setUint8(0, 1, true);
	view.setUint16(2, 2, true);
	view.setUint32(4, 4, true);
	trace(`${ test.abcToString(view.buffer) }\n`);
	
	trace(`${ view.getUint8(0, true) }, ${ view.getUint16(2, true) }, ${ view.getUint32(4, true) }\n`);
} 
catch(error) {
	console.log("Error on new FFI: " + error);
}
