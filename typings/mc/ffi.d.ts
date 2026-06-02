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

// Fallback typing for mc/ffi. When a project's manifest declares ffi.functions,
// mcconfig generates a project-specific mc.ffi.d.ts with the exact signatures
// of the bound native functions; that generated typing takes precedence via
// the per-build tsconfig paths map.

declare module "mc/ffi" {
    interface FFI {
        [name: string]: (...args: any[]) => any;
    }
    const FFI: { new(): FFI };
    export default FFI;
}
