/*
 * Copyright (c) 2026  Moddable Tech, Inc.
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

// legacy numeric codes (https://webidl.spec.whatwg.org/#dfn-error-names-table); a name without one reports 0
const codes = `
IndexSizeError 1
HierarchyRequestError 3
WrongDocumentError 4
InvalidCharacterError 5
NoModificationAllowedError 7
NotFoundError 8
NotSupportedError 9
InUseAttributeError 10
InvalidStateError 11
SyntaxError 12
InvalidModificationError 13
NamespaceError 14
InvalidAccessError 15
TypeMismatchError 17
SecurityError 18
NetworkError 19
AbortError 20
URLMismatchError 21
QuotaExceededError 22
TimeoutError 23
InvalidNodeTypeError 24
DataCloneError 25
`;

class DOMException extends Error {
	constructor(message, name) {
		super(message);
		Object.defineProperty(this, "name", { value:name });
	}
	get code() {
		const index = codes.indexOf(`\n${this.name} `);
		return (index < 0) ? 0 : parseInt(codes.slice(index + this.name.length + 2));
	}
}

export { 
	DOMException,
} 
