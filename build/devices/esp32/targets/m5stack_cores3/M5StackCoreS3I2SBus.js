/*
 * Copyright (c) 2026 Shinya Ishikawa
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

let owner = "";

export function acquireI2S(role, sampleRate) {
	if (owner && (owner !== role))
		releaseI2S(owner);
	if (role === "speaker") {
		if (globalThis.amp && globalThis.amp.start)
			globalThis.amp.start(sampleRate);
	}
	else if (globalThis.mic && globalThis.mic.start)
		globalThis.mic.start(sampleRate);
	owner = role;
}

export function releaseI2S(role) {
	if (owner !== role)
		return;
	if (role === "speaker") {
		if (globalThis.amp && globalThis.amp.stop)
			globalThis.amp.stop();
	}
	else if (globalThis.mic && globalThis.mic.stop)
		globalThis.mic.stop();
	owner = "";
}
