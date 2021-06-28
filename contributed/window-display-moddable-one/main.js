/*
 * Copyright (c) 2016-2020 Moddable Tech, Inc.
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
import SCREENS from "screens";

class AppBehavior extends Behavior {
	onCreate(application) {
		this.index = -1;
		this.switchScreen(application);
	}
	switchScreen(application) {
		this.index += 1;
		if (this.index >= SCREENS.length) this.index = 0;
		application.defer("doSwitchScreen");
	}
	doSwitchScreen(application, nextScreenName, nextScreenData) {
		if (application.length) 
			application.remove(application.first);
		application.purge();
		application.add(new SCREENS[this.index]({}));
	}
}
Object.freeze(AppBehavior.prototype);

export default function() {
	return new Application(null, {
		top: 0, bottom: 0, left: 0, right: 0,
		Behavior: AppBehavior
	});
}
