/*
* Copyright (c) 2019-2020 Bradley Farias
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

declare module "http" {
  type RequestBodyFragment = 0;
  type RequestStatusCode = 1;
  type RequestHeaderReceived = 2;
  type RequestAllHeaders = 3;
  type RequestResponseFragment = 4;
  type RequestAllResponse = 5;
  type RequestStatus = (
    RequestBodyFragment |
    RequestStatusCode |
    RequestHeaderReceived |
    RequestAllHeaders |
    RequestResponseFragment |
    RequestAllResponse
  );
  class Request {
    constructor(options: {
      host?: string,
      address?: string,
      port?: number,
      path?: string,
      method?: string,
      headers?: (string | number)[],
      body?: boolean | string | ArrayBuffer,
      response?: typeof String | typeof ArrayBuffer
    });
    close(): void;
    read(): number;
    read<T extends typeof String | typeof ArrayBuffer>(
      type: T,
      until?: number
    ): T extends typeof String ? string : ArrayBuffer;
    // TODO: better
    callback: (message: RequestStatus, val1?: any, val2?: any) => void;
    
    static requestFragment: number;
    static status: number;
    static header: number;
    static headersComplete: number;
    static responseFragment: number;
    static responseComplete: number;
    static error: number;
  }
  class Server {
    constructor(
      options: import('socket').TCPSocketOptions & {
        port?: number
      }
    );
    close();
    // TODO: better
    callback: (message: number, val1?: any, val2?: any) => void;
  
    static connection: number;
    static status: number;
    static header: number;
    static headersComplete: number;
    static requestFragment: number;
    static requestComplete: number;
    static prepareResponse: number;
    static responseFragment: number;
    static responseComplete: number;
    static error: number;
}
  export {Request, Server};
}
