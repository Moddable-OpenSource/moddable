import FFI from "ffi";

try {
	const test = new FFI;
	
	trace(`test.add(2, 3) = ${ test.add(2, 3) }\n`);
	trace(`test.addSquares(3, 4) = ${ test.addSquares(3, 4) }\n`);
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
