class UDP @ "xs_udp_destructor" {
	constructor(dictionary) @ "xs_udp_constructor";
	close() @ "xs_udp_close"
	read() @ "xs_udp_read"
	write(byte) @ "xs_udp_write"

	get format() {
		return "buffer";
	}
	set format(value) {
		if ("buffer" !== value)
			throw new RangeError;
	}
}
Object.freeze(UDP.prototype);

export default UDP;
