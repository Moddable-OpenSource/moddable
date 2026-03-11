/*
 * Copyright (c) 2025  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Runtime.
 * 
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 * 
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

function resolveHook(specifier, referrer) {
	let dot;
	if (specifier.endsWith(".js"))
		dot = -3;
	else if (specifier.endsWith(".mjs"))
		dot = -4;
	else
		dot = undefined;
	if (specifier.startsWith("./")) {
		let slash = referrer.lastIndexOf('/');
		if (slash >= 0)
			specifier = referrer.slice(0, slash) + specifier.slice(1, dot);
		else
			specifier = specifier.slice(2, dot);
	}
	else if (specifier.startsWith("../")) {
		let c = 3;
		while (specifier.indexOf("../", c) >= 0) {
			c += 3;
		}
		let slash = referrer.lastIndexOf('/');
		if (slash >= 0) {
			for (let i = 0; i < c; i += 3) {
				slash = referrer.lastIndexOf('/', slash - 1);
				if (slash < 0)
					break;
			}
			if (slash >= 0)
				specifier = referrer.slice(0, slash) + specifier.slice(c - 1, dot);
			else
				specifier = specifier.slice(c, dot);
		}
		else
			specifier = specifier.slice(c, dot);
	}
	else if (dot)
		 specifier = specifier.slice(0, dot);
	return specifier;
}

export default class ArchiveCompartment extends Compartment {
	constructor(archive, options) {
		options.globals.archive = archive;
		let specifiers = archive.modulePaths;
		specifiers.forEach(function(path) {
			options.modules[path] = { archive, path }
		});
		options.resolveHook = resolveHook;
		super(options);
		this.specifiers = specifiers;
	}
	has(specifier) {
		return this.specifiers.indexOf(specifier) >= 0;
	}
}
