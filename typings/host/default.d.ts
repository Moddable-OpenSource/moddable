/*
* Copyright (c) 2026 Moddable Tech, Inc.
*
*   This file is part of the Moddable SDK Tools.
*
*   The Moddable SDK Tools is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   The Moddable SDK Tools is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
*
*/

// the host provider typing when no target is named: the generic host plus every standard IO class,
// since without a manifest there is no way to know which are in the build
/// <reference path="./generic.d.ts" />
/// <reference path="../config/analog.d.ts" />
/// <reference path="../config/digital.d.ts" />
/// <reference path="../config/i2c.d.ts" />
/// <reference path="../config/pulsecount.d.ts" />
/// <reference path="../config/pwm.d.ts" />
/// <reference path="../config/serial.d.ts" />
/// <reference path="../config/socket.d.ts" />
/// <reference path="../config/spi.d.ts" />
