import System_ from "embedded:io/system"
import Device from "embedded:provider/builtin"
import { PocoPrototype } from "commodetto/Poco"

export {}
declare global {
  // NOTE: `System` is non-standard and temporary to support the IO examples. Breaking changes are possible.
  var System: typeof System_;
  var device: typeof Device;
  var poco: PocoPrototype;
}
