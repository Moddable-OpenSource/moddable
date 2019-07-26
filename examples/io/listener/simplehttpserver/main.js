import Listener from "builtin/socket/listener";
import TCP from "builtin/socket/tcp";

new Listener({
	port: 80,
	onReadable(count) {
		while (count--) {
			const socket = new TCP({
				from: this.read(),
				onReadable() {
					const response = this.read();

					this.write(ArrayBuffer.fromString("HTTP/1.1 200 OK\r\n"));
					this.write(ArrayBuffer.fromString("connection: close\r\n"));
					this.write(ArrayBuffer.fromString("content-type: text/plain\r\n"));
					this.write(ArrayBuffer.fromString(`content-length: ${response.byteLength}\r\n`));
					this.write(ArrayBuffer.fromString("\r\n"));
					this.write(response);

					this.close();
				},
			});
		}
	}
});
