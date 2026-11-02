import builtin from "embedded:provider/builtin";
import Modules from "modules";

/** @type {import("embedded:provider/builtin").DeviceConfig} */
const device = builtin;

Object.defineProperty(device, "files", {
	get() { return Modules.importNow("embedded:storage/files"); },
	enumerable: true
});
