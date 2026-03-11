/*
* Copyright (c) 2025-2026 Moddable Tech, Inc.
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

type ButtonType = "back" | "up" | "down" | "select";

interface PebbleButtonSingleOptions {
  type: ButtonType;
  onPush: (pushed: 0 | 1, type: ButtonType) => void;
}

interface PebbleButtonMultipleOptions {
  types: ButtonType[];
  onPush: (pushed: 0 | 1, type: ButtonType) => void;
}

type PebbleButtonOptions = PebbleButtonSingleOptions | PebbleButtonMultipleOptions;

declare class PebbleButton {
  constructor(options: PebbleButtonOptions);
  close(): void;
}

export default PebbleButton;
