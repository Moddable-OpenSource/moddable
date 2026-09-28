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

export type ButtonType = "back" | "up" | "down" | "select";

export type RecognizerType = "single" | "long" | "multi" | "raw";

export interface SingleRecognizerOptions {
  repeat?: number;
}

export interface LongRecognizerOptions {
  delay?: number;
}

export interface MultiRecognizerOptions {
  min?: number;
  max?: number;
  lastOnly?: boolean;
  timeout?: number;
}

type OnPush = (pushed: 0 | 1, type: ButtonType, recognizer: RecognizerType, count: number, repeat: boolean) => void;

interface PebbleButtonRecognizers {
  single?: boolean | SingleRecognizerOptions;
  long?: boolean | LongRecognizerOptions;
  multi?: boolean | MultiRecognizerOptions;
  raw?: boolean;
}

interface PebbleButtonSingleOptions extends PebbleButtonRecognizers {
  type: ButtonType;
  onPush: OnPush;
}

interface PebbleButtonMultipleOptions extends PebbleButtonRecognizers {
  types: ButtonType[];
  onPush: OnPush;
}

type PebbleButtonOptions = PebbleButtonSingleOptions | PebbleButtonMultipleOptions;

declare class PebbleButton implements Disposable {
  constructor(options: PebbleButtonOptions);
  close(): void;
  [Symbol.dispose](): void;
}

export default PebbleButton;
