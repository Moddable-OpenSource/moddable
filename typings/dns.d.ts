declare module "dns" {
  // pulled from
  // const impl: typeof import('../modules/network/dns/dns'),
  var DNS: {
    CLASS: {
      IN: number,
      NONE: number,
      ANY: number,
    },
    OPCODE: {
      QUERY: number,
      UPDATE: number,
    },
    RR: {
      A: number,
      PTR: number,
      TXT: number,
      AAAA: number,
      SRV: number,
      OPT: number,
      NSEC: number,
      ANY: number,
    },
    SECTION: {
      QUESTION: number,
      ANSWER: number,
      AUTHORITATIVE: number,
      ADDITIONAL: number,
      ZONE: number,
      PREREQUISITE: number,
      UPDATE: number,
    }
  };
  export {DNS as default};
}