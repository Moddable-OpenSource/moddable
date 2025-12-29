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

declare module "embedded:network/interface/ethernet" {
	import type {PortSpecifier} from "embedded:io/_common";

	export interface EthernetConstructorOptions {
		port?: PortSpecifier;
		onChanged?: () => void;
	}

	class Ethernet {
		constructor(options: EthernetConstructorOptions);

		close(): void;
		connect(options: {}): void;
		disconnect(): void;

		readonly connection: number;
		readonly address: string | undefined;
		readonly MAC: string | undefined;
	}

	export default Ethernet;
}
