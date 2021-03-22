import REPL from "repl";
import Modules from "modules";

const newline = "\n";

globalThis.console = Object.freeze({
	log: function (...str) {
		REPL.write(...str, newline);
	}
}, true);

globalThis.require = Modules.importNow;

const console = new REPL;
