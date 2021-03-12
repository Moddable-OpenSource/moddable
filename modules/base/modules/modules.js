class Modules {
	static get host() @ "xs_modules_host";
	static get archive() @ "xs_modules_archive";
	static importNow(name) @ "xs_modules_importNow";
	static has(name) @ "xs_modules_has";
}

export default Modules;
