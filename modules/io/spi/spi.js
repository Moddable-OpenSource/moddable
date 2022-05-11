/*
 * Copyright (c) 2021  Moddable Tech, Inc.
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

class SPI @ "xs_spi_destructor" {
	constructor(dictionary) @ "xs_spi_constructor"
	close() @ "xs_spi_close"
	read(count) @ "xs_spi_read"
	write(buffer) @ "xs_spi_write"
	transfer(buffer) @ "xs_spi_transfer"
	flush(deselect) @ "xs_spi_flush"

	get format() {
		return "buffer";
	}
	set format(value) {
		if ("buffer" != value)
			throw new RangeError;
	}

	// experimental, to support Display
	set transform() @ "xs_spi_set_transform"
}

export default SPI;
