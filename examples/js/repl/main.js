import REPL from "repl";

globalThis.require = function(specifier) @ "xs_require";

let console = new REPL;
