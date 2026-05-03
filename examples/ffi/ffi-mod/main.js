import FFI from "ffi";
import test from "test";
try {
	test(new FFI);
} 
catch(error) {
	trace(`Error on new FFI: ${ error }\n`);
}
