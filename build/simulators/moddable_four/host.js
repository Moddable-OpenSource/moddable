import Button from "button";
import LED from "led";
import JogDial from "jogdial";

globalThis.Host = Object.freeze({
	Button,
	JogDial,
	LED: { Default: LED },
}, true);