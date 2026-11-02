/*
 * Copyright (c) 2026  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Runtime.
 *
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
	CryptoKit for the ECMA-419 crypto classes on macOS: C entry points (xscryptokit.h) over Apple's Swift-only framework.

	CryptoKit's types are generic Swift structs with no C or Objective-C surface, so each operation is reached through
	a @_cdecl function. Running hashes and HMACs are boxed on the Swift heap and handed to C as opaque pointers; the
	one-shot operations (HKDF, the AEADs, the EC operations) take and return bytes. Nothing here allocates on behalf of C
	beyond those boxes: callers size every output buffer.
*/

import CryptoKit
import Foundation

// an empty buffer keeps a real base address: CryptoKit's DataProtocol paths unwrap it
let emptyBase = UnsafeMutableRawPointer.allocate(byteCount: 1, alignment: 1)

func bytes(_ p: UnsafeRawPointer?, _ count: Int) -> UnsafeRawBufferPointer {
	if (count <= 0) || (nil == p) {
		return UnsafeRawBufferPointer(start: emptyBase, count: 0)
	}
	return UnsafeRawBufferPointer(start: p, count: count)
}

func copyOut<B: ContiguousBytes>(_ value: B, _ out: UnsafeMutableRawPointer, _ max: Int) -> Int {
	return value.withUnsafeBytes { source in
		let count = min(source.count, max)
		if count > 0 {
			out.copyMemory(from: source.baseAddress!, byteCount: count)
		}
		return count
	}
}

// MARK: - hashes (xscryptokit.h: ckHash*)

class HashBox {
	func update(_ data: UnsafeRawBufferPointer) {}
	func finalize(_ out: UnsafeMutableRawPointer) {}
	func reset() {}
}

final class HashBoxOf<H: HashFunction>: HashBox {
	var hash = H()
	override func update(_ data: UnsafeRawBufferPointer) { hash.update(bufferPointer: data) }
	override func finalize(_ out: UnsafeMutableRawPointer) { _ = copyOut(hash.finalize(), out, H.Digest.byteCount) }		// finalize is non-mutating: the running state continues
	override func reset() { hash = H() }
}

func makeHash(_ alg: Int32) -> HashBox? {
	switch alg {
		case 0: return HashBoxOf<Insecure.MD5>()
		case 1: return HashBoxOf<Insecure.SHA1>()
		case 2: return HashBoxOf<SHA256>()
		case 3: return HashBoxOf<SHA384>()
		case 4: return HashBoxOf<SHA512>()
		default: return nil
	}
}

@_cdecl("ckHashCreate")
public func ckHashCreate(_ alg: Int32) -> UnsafeMutableRawPointer? {
	guard let box = makeHash(alg) else { return nil }
	return Unmanaged.passRetained(box).toOpaque()
}

@_cdecl("ckHashUpdate")
public func ckHashUpdate(_ handle: UnsafeMutableRawPointer, _ data: UnsafeRawPointer?, _ count: Int) {
	Unmanaged<HashBox>.fromOpaque(handle).takeUnretainedValue().update(bytes(data, count))
}

@_cdecl("ckHashFinalize")
public func ckHashFinalize(_ handle: UnsafeMutableRawPointer, _ out: UnsafeMutableRawPointer) {
	Unmanaged<HashBox>.fromOpaque(handle).takeUnretainedValue().finalize(out)
}

@_cdecl("ckHashReset")
public func ckHashReset(_ handle: UnsafeMutableRawPointer) {
	Unmanaged<HashBox>.fromOpaque(handle).takeUnretainedValue().reset()
}

@_cdecl("ckHashFree")
public func ckHashFree(_ handle: UnsafeMutableRawPointer) {
	Unmanaged<HashBox>.fromOpaque(handle).release()
}

// MARK: - HMAC (ckHMAC*)

final class HMACBoxOf<H: HashFunction>: HashBox {
	let key: SymmetricKey
	var mac: HMAC<H>
	init(key: SymmetricKey) {
		self.key = key
		mac = HMAC<H>(key: key)
	}
	override func update(_ data: UnsafeRawBufferPointer) { mac.update(data: data) }
	override func finalize(_ out: UnsafeMutableRawPointer) { _ = copyOut(mac.finalize(), out, H.Digest.byteCount) }
	override func reset() { mac = HMAC<H>(key: key) }
}

@_cdecl("ckHMACCreate")
public func ckHMACCreate(_ alg: Int32, _ key: UnsafeRawPointer?, _ keyCount: Int) -> UnsafeMutableRawPointer? {
	let symmetric = SymmetricKey(data: bytes(key, keyCount))
	let box: HashBox
	switch alg {
		case 0: box = HMACBoxOf<Insecure.MD5>(key: symmetric)
		case 1: box = HMACBoxOf<Insecure.SHA1>(key: symmetric)
		case 2: box = HMACBoxOf<SHA256>(key: symmetric)
		case 3: box = HMACBoxOf<SHA384>(key: symmetric)
		case 4: box = HMACBoxOf<SHA512>(key: symmetric)
		default: return nil
	}
	return Unmanaged.passRetained(box).toOpaque()
}

// MARK: - HKDF (ckHKDF): one shot; the caller keeps the output within RFC 5869's limit

func hkdf<H: HashFunction>(_: H.Type, _ mode: Int32, _ ikm: UnsafeRawBufferPointer, _ salt: UnsafeRawBufferPointer, _ info: UnsafeRawBufferPointer, _ out: UnsafeMutableRawPointer, _ outCount: Int) -> Int32 {
	switch mode {
		case 0:
			let key = HKDF<H>.deriveKey(inputKeyMaterial: SymmetricKey(data: ikm), salt: salt, info: info, outputByteCount: outCount)
			return Int32(copyOut(key, out, outCount))
		case 1:
			guard outCount == H.Digest.byteCount else { return -1 }
			let prk = HKDF<H>.extract(inputKeyMaterial: SymmetricKey(data: ikm), salt: salt)
			return Int32(copyOut(prk, out, outCount))
		case 2:
			let key = HKDF<H>.expand(pseudoRandomKey: ikm, info: info, outputByteCount: outCount)
			return Int32(copyOut(key, out, outCount))
		default:
			return -1
	}
}

@_cdecl("ckHKDF")
public func ckHKDF(_ alg: Int32, _ mode: Int32, _ ikm: UnsafeRawPointer?, _ ikmCount: Int, _ salt: UnsafeRawPointer?, _ saltCount: Int, _ info: UnsafeRawPointer?, _ infoCount: Int, _ out: UnsafeMutableRawPointer, _ outCount: Int) -> Int32 {
	let i = bytes(ikm, ikmCount), s = bytes(salt, saltCount), n = bytes(info, infoCount)
	switch alg {
		case 0: return hkdf(Insecure.MD5.self, mode, i, s, n, out, outCount)
		case 1: return hkdf(Insecure.SHA1.self, mode, i, s, n, out, outCount)
		case 2: return hkdf(SHA256.self, mode, i, s, n, out, outCount)
		case 3: return hkdf(SHA384.self, mode, i, s, n, out, outCount)
		case 4: return hkdf(SHA512.self, mode, i, s, n, out, outCount)
		default: return -1
	}
}

// MARK: - AEAD (ckAEADSeal, ckAEADOpen): one shot, 16-byte tag

@_cdecl("ckAEADSeal")
public func ckAEADSeal(_ alg: Int32, _ key: UnsafeRawPointer?, _ keyCount: Int, _ nonce: UnsafeRawPointer?, _ nonceCount: Int, _ aad: UnsafeRawPointer?, _ aadCount: Int, _ input: UnsafeRawPointer?, _ inputCount: Int, _ out: UnsafeMutableRawPointer, _ tag: UnsafeMutableRawPointer) -> Int32 {
	let k = SymmetricKey(data: bytes(key, keyCount))
	let n = bytes(nonce, nonceCount), a = bytes(aad, aadCount), message = bytes(input, inputCount)
	do {
		let ciphertext: Data, mac: Data
		if 0 == alg {
			let box = try AES.GCM.seal(message, using: k, nonce: AES.GCM.Nonce(data: n), authenticating: a)
			ciphertext = box.ciphertext
			mac = box.tag
		}
		else {
			let box = try ChaChaPoly.seal(message, using: k, nonce: ChaChaPoly.Nonce(data: n), authenticating: a)
			ciphertext = box.ciphertext
			mac = box.tag
		}
		guard ciphertext.count == inputCount, mac.count == 16 else { return -1 }
		_ = copyOut(ciphertext, out, inputCount)
		_ = copyOut(mac, tag, 16)
		return 0
	}
	catch {
		return -1
	}
}

// 0: authenticated, plaintext written; 1: authentication failed; -1: invalid key, nonce or tag
@_cdecl("ckAEADOpen")
public func ckAEADOpen(_ alg: Int32, _ key: UnsafeRawPointer?, _ keyCount: Int, _ nonce: UnsafeRawPointer?, _ nonceCount: Int, _ aad: UnsafeRawPointer?, _ aadCount: Int, _ input: UnsafeRawPointer?, _ inputCount: Int, _ tag: UnsafeRawPointer?, _ out: UnsafeMutableRawPointer) -> Int32 {
	let k = SymmetricKey(data: bytes(key, keyCount))
	let n = bytes(nonce, nonceCount), a = bytes(aad, aadCount), ciphertext = bytes(input, inputCount), t = bytes(tag, 16)
	do {
		let plaintext: Data
		if 0 == alg {
			let box = try AES.GCM.SealedBox(nonce: AES.GCM.Nonce(data: n), ciphertext: ciphertext, tag: t)
			plaintext = try AES.GCM.open(box, using: k, authenticating: a)
		}
		else {
			let box = try ChaChaPoly.SealedBox(nonce: ChaChaPoly.Nonce(data: n), ciphertext: ciphertext, tag: t)
			plaintext = try ChaChaPoly.open(box, using: k, authenticating: a)
		}
		guard plaintext.count == inputCount else { return -1 }
		_ = copyOut(plaintext, out, inputCount)
		return 0
	}
	catch CryptoKitError.authenticationFailure {
		return 1
	}
	catch {
		return -1
	}
}

// MARK: - elliptic curves (ckEC*): curves 0 P-256, 1 P-384, 2 X25519
// scalars are the curve's raw private key bytes; points are X9.63 uncompressed (04 || x || y) on the NIST curves and u on X25519

// a digest supplied by the caller, for ECDSA over a digest the caller computed; CryptoKit signs the bytes a Digest yields
protocol RawDigestCount { static var count: Int { get } }
enum Count16: RawDigestCount { static var count: Int { 16 } }
enum Count20: RawDigestCount { static var count: Int { 20 } }
enum Count28: RawDigestCount { static var count: Int { 28 } }
enum Count32: RawDigestCount { static var count: Int { 32 } }
enum Count48: RawDigestCount { static var count: Int { 48 } }
enum Count64: RawDigestCount { static var count: Int { 64 } }

struct RawDigest<N: RawDigestCount>: Digest {
	let value: [UInt8]
	static var byteCount: Int { N.count }
	func withUnsafeBytes<R>(_ body: (UnsafeRawBufferPointer) throws -> R) rethrows -> R { try value.withUnsafeBytes(body) }
	var description: String { "RawDigest(\(N.count))" }
	func hash(into hasher: inout Hasher) { hasher.combine(value) }
	static func == (a: RawDigest<N>, b: RawDigest<N>) -> Bool { a.value == b.value }
}

func rawDigest(_ data: UnsafeRawBufferPointer) -> (any Digest)? {
	let value = [UInt8](data)
	switch value.count {
		case 16: return RawDigest<Count16>(value: value)
		case 20: return RawDigest<Count20>(value: value)
		case 28: return RawDigest<Count28>(value: value)
		case 32: return RawDigest<Count32>(value: value)
		case 48: return RawDigest<Count48>(value: value)
		case 64: return RawDigest<Count64>(value: value)
		default: return nil
	}
}

@_cdecl("ckECGenerate")
public func ckECGenerate(_ curve: Int32, _ scalar: UnsafeMutableRawPointer, _ scalarCount: UnsafeMutablePointer<Int>, _ point: UnsafeMutableRawPointer, _ pointCount: UnsafeMutablePointer<Int>) -> Int32 {
	switch curve {
		case 0:
			let key = P256.Signing.PrivateKey()
			scalarCount.pointee = copyOut(key.rawRepresentation, scalar, scalarCount.pointee)
			pointCount.pointee = copyOut(key.publicKey.x963Representation, point, pointCount.pointee)
		case 1:
			let key = P384.Signing.PrivateKey()
			scalarCount.pointee = copyOut(key.rawRepresentation, scalar, scalarCount.pointee)
			pointCount.pointee = copyOut(key.publicKey.x963Representation, point, pointCount.pointee)
		case 2:
			let key = Curve25519.KeyAgreement.PrivateKey()
			scalarCount.pointee = copyOut(key.rawRepresentation, scalar, scalarCount.pointee)
			pointCount.pointee = copyOut(key.publicKey.rawRepresentation, point, pointCount.pointee)
		default:
			return -1
	}
	return 0
}

@_cdecl("ckECPublic")
public func ckECPublic(_ curve: Int32, _ scalar: UnsafeRawPointer?, _ scalarCount: Int, _ point: UnsafeMutableRawPointer, _ pointCount: UnsafeMutablePointer<Int>) -> Int32 {
	let d = bytes(scalar, scalarCount)
	do {
		switch curve {
			case 0: pointCount.pointee = copyOut(try P256.Signing.PrivateKey(rawRepresentation: d).publicKey.x963Representation, point, pointCount.pointee)
			case 1: pointCount.pointee = copyOut(try P384.Signing.PrivateKey(rawRepresentation: d).publicKey.x963Representation, point, pointCount.pointee)
			case 2: pointCount.pointee = copyOut(try Curve25519.KeyAgreement.PrivateKey(rawRepresentation: d).publicKey.rawRepresentation, point, pointCount.pointee)
			default: return -1
		}
		return 0
	}
	catch {
		return -1
	}
}

// r || s of the curve's size; -1 for an invalid scalar or digest length
@_cdecl("ckECDSASign")
public func ckECDSASign(_ curve: Int32, _ scalar: UnsafeRawPointer?, _ scalarCount: Int, _ digest: UnsafeRawPointer?, _ digestCount: Int, _ out: UnsafeMutableRawPointer, _ outCount: Int) -> Int32 {
	guard let d = rawDigest(bytes(digest, digestCount)) else { return -1 }
	let s = bytes(scalar, scalarCount)
	do {
		switch curve {
			case 0: return Int32(copyOut(try P256.Signing.PrivateKey(rawRepresentation: s).signature(for: d).rawRepresentation, out, outCount))
			case 1: return Int32(copyOut(try P384.Signing.PrivateKey(rawRepresentation: s).signature(for: d).rawRepresentation, out, outCount))
			default: return -1
		}
	}
	catch {
		return -1
	}
}

// 1 valid, 0 invalid, -1 for an invalid point
@_cdecl("ckECDSAVerify")
public func ckECDSAVerify(_ curve: Int32, _ point: UnsafeRawPointer?, _ pointCount: Int, _ digest: UnsafeRawPointer?, _ digestCount: Int, _ signature: UnsafeRawPointer?, _ signatureCount: Int) -> Int32 {
	guard let d = rawDigest(bytes(digest, digestCount)) else { return 0 }
	let p = bytes(point, pointCount), sig = bytes(signature, signatureCount)
	do {
		switch curve {
			case 0:
				let key = try P256.Signing.PublicKey(x963Representation: p)
				guard let s = try? P256.Signing.ECDSASignature(rawRepresentation: sig) else { return 0 }
				return key.isValidSignature(s, for: d) ? 1 : 0
			case 1:
				let key = try P384.Signing.PublicKey(x963Representation: p)
				guard let s = try? P384.Signing.ECDSASignature(rawRepresentation: sig) else { return 0 }
				return key.isValidSignature(s, for: d) ? 1 : 0
			default:
				return -1
		}
	}
	catch {
		return -1
	}
}

// the shared secret (x on the NIST curves, u on X25519); -1 for an invalid scalar or point
@_cdecl("ckECDH")
public func ckECDH(_ curve: Int32, _ scalar: UnsafeRawPointer?, _ scalarCount: Int, _ point: UnsafeRawPointer?, _ pointCount: Int, _ out: UnsafeMutableRawPointer, _ outCount: Int) -> Int32 {
	let s = bytes(scalar, scalarCount), p = bytes(point, pointCount)
	do {
		let secret: SharedSecret
		switch curve {
			case 0: secret = try P256.KeyAgreement.PrivateKey(rawRepresentation: s).sharedSecretFromKeyAgreement(with: P256.KeyAgreement.PublicKey(x963Representation: p))
			case 1: secret = try P384.KeyAgreement.PrivateKey(rawRepresentation: s).sharedSecretFromKeyAgreement(with: P384.KeyAgreement.PublicKey(x963Representation: p))
			case 2: secret = try Curve25519.KeyAgreement.PrivateKey(rawRepresentation: s).sharedSecretFromKeyAgreement(with: Curve25519.KeyAgreement.PublicKey(rawRepresentation: p))
			default: return -1
		}
		return Int32(copyOut(secret, out, outCount))
	}
	catch {
		return -1
	}
}
