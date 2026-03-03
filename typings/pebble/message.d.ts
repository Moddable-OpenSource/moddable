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

interface MessageOptions {
  onReadable?: () => void;
  onWritable?: (count: number) => void;
  onSuspend?: () => void;
  format?: "map";
  input?: number;
  output?: number;
  keys?: Map<string, number> | Array<string>;
}

type MessageKey = string | number;
type MessageReadValue = number | string | ArrayBuffer;
type MessageWriteValue = number | string | ByteBuffer | boolean;

declare class Message {
  constructor(options: MessageOptions);
  close(): void;
  read(): Map<MessageKey, MessageReadValue>;
  write(map: Map<MessageKey, MessageWriteValue>): void;
  get format(): "map";
  set format(value: "map");
  get input(): number;
  get output(): number;
}

export default Message;
