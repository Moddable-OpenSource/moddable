/*
 * Copyright (c) 2021-2022  Moddable Tech, Inc.
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

import EventSource from "eventsource";

/*
git clone https://github.com/mdn/dom-examples.git
cd dom-examples/server-sent-events
php -S 127.0.0.1:8000
*/

const source = new EventSource("http://127.0.0.1:8000/sse.php");

source.onerror = function() {
	trace("EventSource failed.\n");
};
source.onopen = function() {
	trace("Connection to server opened.\n");
};
source.onmessage = function(e) {
	trace("message: " + e.data + "\n");
};
source.addEventListener("ping", function(e) {
	trace("ping: " + e.data + "\n");
});
