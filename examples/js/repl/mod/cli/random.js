import CLI from "cli";

CLI.install(function(command, parts) {
	switch (command) {
		case "random":
			const range = parseInt(parts[0] ?? 100);
			this.line(`Random number: ${randomInteger(range)}`);
			return true;

		case "help":
			this.line("random [max] - random integer");
			break;

		default:
			return false;
	}
});
