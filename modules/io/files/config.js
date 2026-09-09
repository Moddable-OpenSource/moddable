import device from "embedded:provider/builtin";
import Modules from "modules";

Object.defineProperty(device, "files", {
	get() { return Modules.importNow("embedded:storage/files"); },
	enumerable: true
});
