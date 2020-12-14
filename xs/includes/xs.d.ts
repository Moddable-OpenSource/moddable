interface Trace {
	(...log: (string | number | boolean)[]):void;
	left(log: string | ArrayBuffer, conversation?: string):void;
	center(log: string | ArrayBuffer, conversation?: string):void;
	right(log: string | ArrayBuffer, conversation?: string):void;
}
declare const trace:Trace;

interface HostBuffer {
}

interface ObjectConstructor {
	freeze<T>(obj: T, freeze?: boolean | number): Readonly<T>;
}

interface StringConstructor {
	fromArrayBuffer(buffer: ArrayBuffer): string;
}

interface ArrayBufferConstructor {
	fromString(string: string): ArrayBuffer;
}

interface JSON {
	parse(text: string, reviver?: string[]): any;
}

// Compartment?