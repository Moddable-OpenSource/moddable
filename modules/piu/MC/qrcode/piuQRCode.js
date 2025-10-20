import QRCodeBuffer from "qrcode";

const qrCode = {
	__proto__: Content.prototype,
	_create($, it) { return native("PiuQRCode_create").call(this, $, it); },
	QRCodeBuffer,
	get string() { return native("PiuQRCode_get_string").call(this); },
	set string(it) { native("PiuQRCode_set_string").call(this, it); },
};
export const QRCode = Template(qrCode);
Object.freeze(qrCode);
globalThis.QRCode = QRCode;
export default QRCode;