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
