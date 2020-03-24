import config from "mc/config";
import JogDial from "jogdial";

const jog = new JogDial({
	jogdial: config.jogdial,
	onTurn(delta) {
		trace(`Turn ${delta}\n`);
	},
	onPushAndTurn(delta) {
		trace(`Push and Turn ${delta}\n`);
	},
	onPush(value) {
		trace(`Button ${value}\n`);
	}
});
