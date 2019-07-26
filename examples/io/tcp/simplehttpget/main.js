import TCP from "builtin/socket/tcp";

class HTTPGet {
	#host;
	#path;
	#port;
	#socket;
	#onData;
	#onError;
	constructor(dictionary) {
		this.#host = dictionary.host;
		this.#path = dictionary.path;
		this.#port = dictionary.port || 80;

		this.#onData = dictionary.onData || this.onData;
		if (!this.#onData)
			throw new Error("onData required");
		this.#onError = dictionary.onError || this.onError;

		System.resolve(this.#host, (host, address) => {
			if (!address) {
				if (this.#onError)
					this.#onError();
				return;
			}

			this.#socket = new TCP({
				address,
				port: this.#port,
				onReadable: this.readable.bind(this),
				onWritable: this.writeable.bind(this),
				onError: this.error.bind(this),
			});
		});
	}
	readable(byteLength) {
		const buffer = this.#socket.read(byteLength);
		this.#onData(buffer);
	}
	writeable(byteLength) {
		if (this.#path) {
			const headers = [
				`GET ${this.#path} HTTP/1.1`,
				`Host: ${this.#host}`,
				"Connection: close",
				"",
				"",
			].join("\r\n")
			this.#socket.write(ArrayBuffer.fromString(headers));

			this.#path = undefined;
		}
	}
	error() {
		if (this.#onError)
			this.#onError();
		this.#socket.close();
		this.#socket = undefined;
	}
}

let request = new HTTPGet({
	host: "httpbin.org",
	port: 80,
	path: "/",
	onData(buffer) {
		trace(String.fromArrayBuffer(buffer));
	},
	onError() {
		trace("\n\n** DONE **\n\n");
	}
});
