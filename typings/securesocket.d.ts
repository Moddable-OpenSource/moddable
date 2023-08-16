/*
* Copyright (c) 2023 Richard Lea
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

declare module "securesocket" {
    import { Socket } from "socket";

    type WriteData = number | string | BufferLike;

    export default class SecureSocket {
        sock: typeof Socket;

        constructor(dict: { [k: string]: any });

        read<T extends typeof String | typeof ArrayBuffer>(
            type: T,
            until?: number
        ): T extends typeof String ? string : ArrayBuffer;
        write(data: WriteData, ...moreData: WriteData[]): void;
        close(): void;
    }
}
