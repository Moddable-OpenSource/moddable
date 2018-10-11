/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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

import Net from "net";

["moddable.tech", "apple.com", "cannot.find.domain", "playground.global"].forEach(domain => {
	Net.resolve(domain, (name, address) => {
		if (address)
			trace(`"${name}" resolved to ${address}\n`);
		else
			trace(`"${name}" could not be resolved\n`);
	});
});
