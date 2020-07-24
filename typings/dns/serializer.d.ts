declare module "dns/serializer" {
  import DNS from "dns";
  class Serializer {
    constructor(options: {
      opcode?: (typeof DNS.OPCODE)[keyof (typeof DNS.OPCODE)],
      query?: boolean,
      authoritative?: boolean,
      id?: number
    });
    add(
      section: (typeof DNS.SECTION)[keyof (typeof DNS.SECTION)],
      name: string,
      type: (typeof DNS.RR)[keyof (typeof DNS.RR)],
      classType: (typeof DNS.CLASS)[keyof (typeof DNS.CLASS)],
      ttl: number,
      data?: ArrayBuffer
    ): void;
    build(): ArrayBuffer;
  }
  export {Serializer as default};
}
