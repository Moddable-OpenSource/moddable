import { ReadableStream, WritableStream } from "streams";

export function IOReadableStreamMixin(Base) {
	return class extends ReadableStream {  
		constructor(dictionary) {
	 		super({
				start(controller) {
					trace(`start readable\n`);
					this.io = new Base({
						...dictionary,
						onReadable() {
							controller.enqueue(this.read());
						}
					});
				}
			})
		}
	};
}

export function IOWritableStreamMixin(Base) {
	return class extends WritableStream {  
		constructor(dictionary) {
	 		super({
				start(controller) {
					trace(`start readable\n`);
					this.io = new Base({
						...dictionary
					});
				},
				write(chunk) {
					this.io.write(chunk);
				}
			})
		}
	};
}