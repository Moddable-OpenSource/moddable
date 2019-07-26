import TCP from "builtin/socket/tcp"

class Listener @ "xs_listener_destructor_" {
	constructor(dictionary) @ "xs_listener_constructor";
	close() @ "xs_listener_close_"
	read() {
		return read.call(this, new TCP);
	}

	get format() {
		return "socket/tcp";
	}
	set format(value) {
		if ("socket/tcp" !== value)
			throw new RangeError;
	}
}
Object.freeze(Listener.prototype);

function read() @ "xs_listener_read";

export default Listener;
