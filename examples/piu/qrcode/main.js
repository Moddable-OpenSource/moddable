/*
 * Copyright (c) 2022 Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import {} from "piu/MC";

new Application(null, {
	skin: new Skin({
		fill: "silver"
	})
});

application.add(new QRCode(null, {
	width: 160, height: 160,
	skin: {
		fill: "white",
		stroke: "black"
	},
	string: "This is a message.",
	maxVersion: 10
}));
