class Serial @ "xs_serial_destructor" {
	constructor(dictionary) @ "xs_serial_constructor"
	close() @ "xs_serial_close"

	read() @ "xs_serial_read"
	write() @ "xs_serial_write"

	get format() @ "xs_serial_get_format"
	set format() @ "xs_serial_set_format"
}
Object.freeze(Serial.prototype);

export default Serial;
