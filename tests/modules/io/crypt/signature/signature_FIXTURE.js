import Signature from "embedded:crypt/signature";
export default Signature;
export * from "../key/rsa_FIXTURE.js";

// the script backend honors any saltLength; the PSA and SecKey backends sign with a salt of the hash length only
export const pssAnySalt = Signature.supports({algorithm: "RSA-PSS", hash: "SHA-256", saltLength: 0});
