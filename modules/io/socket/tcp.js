class TCP @ "xs_tcp_destructor" {
	constructor(dictionary) @ "xs_tcp_constructor";
	close() @ "xs_tcp_close"
	read() @ "xs_tcp_read"
	write(byte) @ "xs_tcp_write"

	get format() @ "xs_tcp_get_format"
	set format() @ "xs_tcp_set_format"
}
Object.freeze(TCP.prototype);

export default TCP;
