import QRCodeBuffer from "qrcode";

const qrCode = {
	__proto__: Content.prototype,
	_create($, it) @ "PiuQRCode_create",
	QRCodeBuffer,
	get string() @ "PiuQRCode_get_string",
	set string(it) @ "PiuQRCode_set_string",
};
export const QRCode = Template(qrCode);
Object.freeze(qrCode);
globalThis.QRCode = QRCode;
export default QRCode;