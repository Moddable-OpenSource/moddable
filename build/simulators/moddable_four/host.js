import Accelerometer from "accelerometer";
import Button from "button";
import LED from "led";
import JogDial from "jogdial";
import Power from "power";

globalThis.Host = Object.freeze({
	Accelerometer,
	Button,
	JogDial,
	LED: { Default: LED },
	Power,
}, true);