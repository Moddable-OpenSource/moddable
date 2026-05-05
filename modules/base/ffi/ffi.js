class FFI extends Native("FFI_destructor") {
	constructor() { super(); native("FFI_constructor").call(this); }
}
export default FFI;