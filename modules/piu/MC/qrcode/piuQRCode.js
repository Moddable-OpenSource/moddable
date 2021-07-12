import QRCodeBuffer from "qrcode";

const qrCode = {
	__proto__: Content.prototype,
	_create($, it) @ "PiuQRCode_create",
	QRCodeBuffer,
};
export const QRCode = Template(qrCode);
Object.freeze(qrCode);
globalThis.QRCode = QRCode;
export default QRCode;