const inverter = {
	__proto__: Content.prototype,
	_create($, it) { return native("PiuInverter_create").call(this, $, it); },
};
export const Inverter = Template(inverter);
Object.freeze(inverter);
globalThis.Inverter = Inverter;
export default Inverter;