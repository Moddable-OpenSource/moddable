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
  type RequestError = -1;
  type RequestBodyFragment = 0;
  type RequestStatusCode = 1;
  type RequestHeaderReceived = 2;
  type RequestAllHeaders = 3;
  type RequestResponseFragment = 4;
  type RequestAllResponse = 5;
  type RequestStatus = (
    RequestError |
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
    
    static readonly requestFragment: RequestBodyFragment;
    static readonly status: RequestStatusCode;
    static readonly header: RequestHeaderReceived;
    static readonly headersComplete: RequestAllHeaders;
    static readonly responseFragment: RequestAllHeaders;
    static readonly responseComplete: RequestAllResponse;
    static readonly error: RequestError;
  }


type ServerConnection = 1;
type ServerStatus = 2;
type ServerHeader = 3;
type ServerHeadersComplete = 4;
type ServerRequestFragment = 5;
type ServerRequestComplete = 6;
type ServerPrepareResponse = 8;
type ServerResponseFragment = 9;
type ServerResponseComplete = 10;
type ServerError = -1;
type ServerMessages = (
  ServerConnection |
  ServerStatus |
  ServerHeader |
  ServerHeadersComplete |
  ServerRequestFragment |
  ServerRequestComplete |
  ServerPrepareResponse |
  ServerResponseFragment |
  ServerResponseComplete |
  ServerError
);

  class Server {
    constructor(
      options?: import('socket').TCPSocketOptions & {
        port?: number
      }
    );
    close();
    read(): number;
    read<T extends typeof String | typeof ArrayBuffer>(
      type: T,
      until?: number
    ): T extends typeof String ? string : ArrayBuffer;
    // TODO: better
    callback: (message: ServerMessages, val1?: any, val2?: any) => void;
  
    static connection: ServerConnection;
    static status: ServerStatus;
    static header: ServerHeader;
    static headersComplete: ServerHeadersComplete;
    static requestFragment: ServerRequestFragment;
    static requestComplete: ServerRequestComplete;
    static prepareResponse: ServerPrepareResponse;
    static responseFragment: ServerResponseFragment;
    static responseComplete: ServerResponseComplete;
    static error: ServerError;
}
  export {Request, Server};
}
