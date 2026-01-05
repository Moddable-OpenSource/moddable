class Encode {
	static toAlaw(input, output) { return native("xs_alaw_encode").call(this, input, output); }
}

export {Encode}
