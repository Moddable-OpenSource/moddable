export class HX711 @ "xs_HX711_destructor" {
	constructor(dictionary) @ "xs_HX711";

	/* accessor */
	get offset() @ "xs_HX711_get_offset"
	set offset(offset) @ "xs_HX711_set_offset"
	get scale() @ "xs_HX711_get_scale"
	set scale(scale) @ "xs_HX711_set_scale"

	// readonly
	get value() @ "xs_HX711_get_value";
	get rawValue() @ "xs_HX711_get_raw_value"
	get clk() @ "xs_HX711_get_clk_pin"
	get dat() @ "xs_HX711_get_dat_pin"
	resetOffset() {
		this.offset = this.rawValue
	}
};

export default HX711;
