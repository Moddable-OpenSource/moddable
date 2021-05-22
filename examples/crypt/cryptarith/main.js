/*
 * Copyright (c) 2016-2019  Moddable Tech, Inc.
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

import Modular from "modular";
import Mont from "mont"
import EC from "ec";
import ECPoint from "ecp";
import RNG from "rng";
import ECDSA from "ecdsa";
import {Digest} from "crypt";

const ps = "0xF02E5376E29C1DA5092ECD2709FC48EBE1350D9E53EBE1B8ED282BB6ED1D2D126ACB47E6CF4D3ED32941EB389EB824AE77D3A4E374A3851A6E54FA7DA217266C0CD20E333841578CB071AE27A87C6224DCE7F582BE8CD6527F01F1A4A5E8F69CA2347C24273C3E47D68EC218E387D2A19A5850AD854C205DF75D0C18CB408ABEF9B04CD254313D5F69E129034C6A0C8C27AB776294F489732591FF90F578FFF6B3BA8481570CA4CBBDBE6A38EC05E2320522597FF60013302952AC8936C9D4167601CA530BC1AD29DB466AF8BA6850CFAC6EBF9D904FAB818D68AB190887DF84DFFFA7F279780FD0A807B599D4AB696F721C2063C7DDE800C86D958ED4EB554B";

const Ms = "0x81a4f3065cdb6c48347cfeb60ba228e134138a942bee5cdb47f6726f5f44e3fa414fca8c5fb7b477e4870eac58989ae1cf0c0c87aebb2726b4c8cd97538091340d3dc5213d0c95c2b51d8e61fa8cf61113faad92723b4458fe1c63d78bed73143ca77b3835d9e31ba03903e859c53b5172b614f9d2b961c64b3d4c533fc00da487c13edf18a4ccf82f564548d07fee137f71f5621be97daae2cdc39c001eb26b2f882cc6a86cf3ead84253f2c44ba1608d97ae98c14d01f78ddfdd8d5234c0013ab2efde0fb677080dfa48d81b49b3746246942229c607e2b3f8b95dd49facaab3d7c69a330b6da75dacef20f06af9734aedac3b873f4618ecab8dcc0bbfc86a"

const es = "0x12bb1c1f0e682befbc092ed035247431264ebee06ab8ac3efffc1b9d081a672cec2ca1d896a01cd1fbe19d5861c0a74775ca05b2f45923609cf030893c2e29ce2008fa8139ed6cc2689abf6645b1065f404babcf4a8e8f54ed2d732c9ea16f4ece0448d75e6062fca7f565bdfee51611b6e370efdc42c44ea06f9f275763c573a75cfa854ac0d745b4f700c12511dcfb14bcc42292b66dd5d30ca0959cf2ec8c6159871829f3242a547ed9d5dd6eac431e2619d08859cfa90003eec632716ee2d30e09cdee769c538758ca6b2d3e7fbbadf58021e74224728770399be5eda9018bd29d5c601892619e76d2a7d382b60a5e7dc61ab06153a86de90b69cb9acc17"

const rs = "0xbb587b89e740bed6c638c01c9c49bb5971db62af6928a78fcd755be5a7e0c1fcfe1479bffa9961d8f9477ee413fc452b411dd675489db2e28f881098cd3b6494cf50670dd7ba3c64afd24909f9033d22ad5e1cf818d761a4cce22076a2c41aca1165ba09f39dfeec10b179754dab8f2f1c4e2fa51e9f0ccc1de47b9a2cfe57c3098a0647e9938104ec9e4c8b3f1a63e5e784c20a26b4df204964f74dcca925ad085fd2faa8be97ca7c0685015a68beb6b0e8c54cedcbe531faa29bfb272bf7a27494b46a2e68cdc4a6f7d6bc767caeca453c920a5f124463dd15e79512f91ecb9e7ce9da367e7f67b8d7d05b34c3f4c91128f6ced8c07f5dcb10317a910957c1"

const kRepeeat = 1;

function test() {
	let p = BigInt(ps);
	//let es = RNG.get(2048/8);
	//let Ms = RNG.get(2048/8);
	let e = BigInt(es);
	let M = BigInt(Ms);
	let r = BigInt(rs);

	trace("p = " + p.toString(16) + "\n");
	trace("M = " + M.toString(16) + "\n");
	trace("e = " + e.toString(16) + "\n");

	let mod = new Modular(p);
	let mont = new Mont({m: p, method: Mont.SW});

	let x
	let t1 = Date.now();
	let n = kRepeeat;
	while (--n >= 0) {
		x = mod.mulinv(M);
	}
	let t2 = Date.now();
	trace(x.toString(16) + "\n");
	trace("mod.mulinv: " + (t2 - t1) + "\n");
/*
	t1 = Date.now();
	n = kRepeeat;
	while (--n >= 0) {
		x = mont.mulinv(M);
	}
	t2 = Date.now();
	trace(x.toString(16) + "\n");
	trace("mont.mulinv: " + (t2 - t1) + "\n");

	t1 = Date.now();
	n = kRepeeat;
	while (--n >= 0) {
		x = mod.exp(M, e);
	}
	t2 = Date.now();
	trace("mod.exp: " + (t2 - t1) + "\n");
	if (r != x)
		trace("failed!\n");

	t1 = Date.now();
	n = kRepeeat;
	while (--n >= 0) {
		x = mont.exp(M, e);
	}
	t2 = Date.now();
	trace("mont.exp: " + (t2 - t1) + "\n");
	if (r != x)
		trace("failed!\n");
*/
	//
	// ECDSA test vectors from
	// https://tools.ietf.org/html/rfc6979
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
	let ks = "0x5FA81C63109BADB88C1F367B47DA606DA28CAD69AA22C4FE6AD7DF73A7173AA5";

	let m = BigInt(ms);
	let a = BigInt(as);
	let b = BigInt(bs);
	let Gx = BigInt(Gxs);
	let Gy = BigInt(Gys);
	let X = BigInt(Xs);

	let P2 = new ECPoint(BigInt("0x7CF27B188D034F7E8A52380304B51AC3C08969E277F21B35A60B48FC47669978"), BigInt("0x07775510DB8ED040293D9AC69F7430DBBA7DADE63CE982299E04B79D227873D1"));
	let P3 = new ECPoint(BigInt("0x5ECBE4D1A6330A44C8F7EF951D4BF165E6C6B721EFADA985FB41661BC6E7FD6C"), BigInt("0x8734640C4998FF7E374B06CE1A64A2ECD82AB036384FB83D9A79B127A27D5032"));

	let G = new ECPoint(Gx, Gy);
	let ec = new EC(a, b, m);

	trace("G = (" + G + ")\n");

	let Q3 = new ECPoint(0n, 0n, true);
	trace("G + 0 = " + ec.add(G, Q3) + "\n");

	let Q2 = ec.mul(G, 2n);
	trace("Q2 = (" + Q2 + ")\n");

	let Q1 = ec.add(G, G);
	trace("Q1 = (" + Q1 + ")\n");
	if (Q1.X != Q2.X || Q1.Y != Q2.Y) {
		trace("doesn't match!\n");
		debugger;
	}

	let P = ec.add(G, P2);
	if (P.X != P3.X || P.Y != P3.Y) {
		trace("doesn't match!\n");
		debugger;
	}

	t1 = Date.now();
	n = 10;
	while (--n >= 0)
		P = ec.mul(G, X);
	t2 = Date.now();
	trace("ec.mul: " + (t2 - t1) + "\n");
	if (P.X != BigInt(Uxs) || P.Y != BigInt(Uys))
		trace("ec.mul failed!\n");
}

export default function() {
	test();
}
