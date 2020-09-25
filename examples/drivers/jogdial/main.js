new Host.JogDial({
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
