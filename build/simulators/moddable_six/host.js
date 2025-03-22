import Backlight from "backlight";
import Button from "button";
import LED from "led";

globalThis.Host = Object.freeze({
	Backlight,
	Button: { Default: Button },
	LED: { Default: LED },
}, true);