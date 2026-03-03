/*
* Copyright (c) 2019-2020 Bradley Farias
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

declare module "dns/parser" {
  interface DNSQuestion {
      qname: string[];
      qtype: number;
      qclass: number;
  }

  interface DNSSRVData {
      priority: number;
      weight: number;
      port: number;
      target: string[];
  }

  interface DNSAnswerA {
      qname: string[];
      qtype: 0x0001;
      qclass: number;
      ttl: number;
      rdata: string;
  }

  interface DNSAnswerPTR {
      qname: string[];
      qtype: 0x000C;
      qclass: number;
      ttl: number;
      rdata: string[];
  }

  interface DNSAnswerTXT {
      qname: string[];
      qtype: 0x0010;
      qclass: number;
      ttl: number;
      rdata: string[];
  }

  interface DNSAnswerSRV {
      qname: string[];
      qtype: 0x0021;
      qclass: number;
      ttl: number;
      rdata: DNSSRVData;
  }

  interface DNSAnswerOther {
      qname: string[];
      qtype: number;
      qclass: number;
      ttl: number;
      rdata: "UNHANDLED";
  }

  type DNSAnswer = DNSAnswerA | DNSAnswerPTR | DNSAnswerTXT | DNSAnswerSRV | DNSAnswerOther;
    
  class Parser {
    constructor(buffer: ByteBuffer);
    get id(): number;
    get flags(): number;
    get questions(): number;
    get answers(): number;
    get authorities(): number;
    get additionals(): number;
    question(index: number): DNSQuestion;
    answer(index: number): DNSAnswer;
    authority(index: number): DNSAnswer;
    additional(index: number): DNSAnswer;
  }
  export {Parser as default};
}
