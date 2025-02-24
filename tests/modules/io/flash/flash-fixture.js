import flash from "embedded:storage/flash"

const path = "xs_test";

const f = globalThis.device?.flash ?? flash;

export {f as default, path}
