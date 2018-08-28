/*
 *     Copyright (C) 2016-2017 Moddable Tech, Inc.
 *     All rights reserved.
 */

import ASSETS from "assets";

export const Spinner = Container.template($ => ({
	left: 0, right: 0, top: 0, bottom: 0, Skin: ASSETS.WhiteSkin,
	contents: [
		new ASSETS.LoadingSpinner({ top: 70, left: 110, squareSize: 20, frequency: 1000, color: [ 230, 0.74, 0.38 ] }) // ~#192eab
	],
}));


