export default function () {
	const archive = globalThis.archive;
	let application;
	if (archive) {
		const compartment = new Compartment({
			globals: { 
				Date, 
				Math, 
			},
			modules: {
				main: { archive, path:"main" },
			},
			loadHook(specifier) {
				return {namespace: specifier};
			},
			loadNowHook(specifier) {
				return {namespace: specifier};
			},
			resolveHook(specifier) {
				return specifier;
			}
		});
		compartment.importNow("main").default;
	}
	else 
		trace("No mod installed\n");
}
