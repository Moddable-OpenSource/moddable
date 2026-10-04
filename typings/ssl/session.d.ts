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

declare module "ssl/session" {
  // the socket a session reads and writes through: the session consumes readable as it reads, and updates writable as it writes
  export interface SSLSessionSocket {
    readable: number;
    writable: number;
    read(buffer: Uint8Array): unknown;
    write(buffer: ByteBuffer): number;
  }

  export interface SSLSessionOptions {
    protocolVersion?: number;
    minProtocolVersion?: number;
    maxProtocolVersion?: number;
    certificate?: ByteBuffer | ByteBuffer[];		// trusted CA certificates
    clientCertificates?: ByteBuffer[];
    clientKey?: ByteBuffer;
    verify?: boolean;
    trace?: boolean;
    cache?: boolean;
    clientAuth?: { cipherSuites: unknown; subjectDN: unknown };
    serverName?: string;							// session_original: alias of tls_server_name
    applicationLayerProtocolNegotiation?: string | ByteBuffer | (string | ByteBuffer)[];	// session_original: alias
    tls_server_name?: string;
    tls_max_fragment_length?: number;
    tls_application_layer_protocol_negotiation?: string | ByteBuffer | (string | ByteBuffer)[];
    tls_elliptic_curves?: number[];
    tls_signature_algorithms?: { hash: number; sig: number }[];
    [extension: `tls_${string}`]: unknown;			// the handshake sends every tls_ option it recognizes as an extension
  }

  class SSLSession {
    constructor(options?: SSLSessionOptions);
    initiateHandshake(socket?: SSLSessionSocket): void;
    handshake(socket: SSLSessionSocket, count?: number): boolean;	// true when complete
    read(socket: SSLSessionSocket, count?: number): Uint8Array | null | undefined;	// undefined: nothing yet; null: closed
    write(socket: SSLSessionSocket, data: ByteBuffer, options?: { more?: boolean; byteLength?: number }): number;
    close(socket: SSLSessionSocket): void;
  }
  export default SSLSession;
}
