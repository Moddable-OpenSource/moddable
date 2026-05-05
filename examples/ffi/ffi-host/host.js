/*
 * Copyright (c) 2026  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

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
