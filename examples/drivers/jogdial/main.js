import JogDial from "pins/jogdial";

const jog = new JogDial({
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
