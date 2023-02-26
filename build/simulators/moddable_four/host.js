import Accelerometer from "accelerometer";
import Button from "button";
import LED from "led";
import JogDial from "jogdial";

globalThis.Host = Object.freeze({
	Accelerometer,
	Button,
	JogDial,
	LED: { Default: LED },
}, true);