/*
 * Copyright (c) 2016-2021  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 *
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import ECPoint from "ecp";
import EC from "ec";
import RNG from "rng";
import ECDSA from "ecdsa";
import {Digest} from "crypt";
import Curve from "curve";

//
// ECDSA test vectors from
// https://tools.ietf.org/html/rfc6979
//

//
// secp256r1
//
let ms = "0xFFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFF";
let as = "0xFFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFC";
let bs = "0x5ac635d8aa3a93e7b3ebbd55769886bc651d06b0cc53b0f63bce3c3e27d2604b";
let Gxs = "0x6b17d1f2e12c4247f8bce6e563a440f277037d812deb33a0f4a13945d898c296";	// generator
let Gys = "0x4fe342e2fe1a7f9b8ee7eb4a7c0f9e162bce33576b315ececbb6406837bf51f5";
let ns = "0xFFFFFFFF00000000FFFFFFFFFFFFFFFFBCE6FAADA7179E84F3B9CAC2FC632551";	// order
let Xs = "0xC9AFA9D845BA75166B5C215767B1D6934E50C3DB36E89B127B8A622B120F6721";	// private key
let Uxs = "0x60FED4BA255A9D31C961EB74C6356D68C049B8923B61FA6CE669622E60F29FB6";	// public key
let Uys = "0x7903FE1008B8BC99A41AE9E95628BC64F2F1B20C2D7E9F5177A3C294D4462299";
let message = "sample";
let ks = "0xA6E3C57DD01ABE90086538398355DD4C3B17AA873382B0F24D6129493D8AAD60";

let rs = "0xEFD48B2AACB6A8FD1140DD9CD45E81D69D2C877B56AAF991C34D0EA84EAF3716";
let ss = "0xF7CB1C942D657C41D436C7A1B6E29F65F3E900DBB9AFF4064DC4AB2F843ACDA8";

let m = BigInt(ms);
let a = BigInt(as);
let b = BigInt(bs);
let Gx = BigInt(Gxs);
let Gy = BigInt(Gys);
let n = BigInt(ns);
let X = BigInt(Xs);
let k = BigInt(ks);
let r = BigInt(rs);
let s = BigInt(ss);

let G = new ECPoint(Gx, Gy);
let ec = new EC(a, b, m);
let P = ec.mul(G, X);
if (P.X != BigInt(Uxs) || P.Y != BigInt(Uys))
	trace("ec.mul failed!\n");

let curve = new Curve("secp256r1");
let Ps = curve.dh(ArrayBuffer.fromBigInt(X));
trace("curve.dh: " + (BigInt.fromArrayBuffer(Ps)).toString(16) + "\n");
let Z = curve.Z(Ps);
if (BigInt.fromArrayBuffer(Z) != BigInt(Uxs))
	trace("cuve.dh failed: " + (BigInt(Z)).toString(16) + "\n");

let key = {
	G: G,
	n: n,
	p: m,
	a: a,
	b: b,
	Qu: P,
	du: X,
	k: k,
};
curve.k = k;
let ecdsa = new ECDSA(X, curve, true);

let digest = new Digest("SHA256");
let H = digest.process(message);
let sig = ecdsa.sign(H);
trace("sig: " + (BigInt.fromArrayBuffer(sig)).toString(16) + "\n")
let l = (BigInt.bitLength(n) + 7) >>> 3;

if (r == BigInt.fromArrayBuffer(sig.slice(0, l)) &&
    s == BigInt.fromArrayBuffer(sig.slice(l, l*2))) {
	let ecdsa = new ECDSA(P, curve, false);
	trace("ecdsa: " + ecdsa.verify(H, sig) ? "succeeded\n" : "failed\n");
}
else {
	trace("ecdsa: failed!\n");
	trace("r = " + r.toString(16) + "\n");
	trace("s = " + s.toString(16) + "\n");
}
