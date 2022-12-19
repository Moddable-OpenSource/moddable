const modules = {
	mod: { source:"mod" },
};
let compartment = new Compartment({
	resolveHook(specifier, refererSpecifier) {
		return specifier;
	},
	loadNowHook(specifier) {
		return modules[specifier];
	}
});
try {
	compartment.importNow("mod")
}
catch(error) {
	trace(error + "\n")
}
