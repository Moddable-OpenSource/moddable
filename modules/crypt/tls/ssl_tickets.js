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
	TLS 1.3 session tickets (RFC 8446 section 4.6.1), kept for resumption (section 4.2.11) in the ECMA-419 Key-Value store
	the socket opened from its tickets option. One opaque entry per host, under a key derived from the host name (store
	backends limit key length: NVS allows 15 characters). The entry holds everything the next ClientHello needs: when the
	ticket arrived, its lifetime and age obfuscator, the hash of the suite it was issued under, the ticket, and the PSK
	derived from the resumption master secret. An expired entry is deleted when read. A write also sweeps the domain:
	expired entries go, then the least recently written beyond options.ticketsMax. The write path runs once per
	connection, after the handshake, where that cost is invisible; the read path is one read.
*/
import Digest from "embedded:crypt/digest";
import {hkdfExpandLabel, hashSize} from "ssl/keyschedule";

const maximumLifetime = 604800;		// seconds: the most a server may grant (section 4.6.1)
const header = 19;		// received (float64 ms), lifetime (uint32 s), ticket_age_add (uint32), hash (uint8), ticket length (uint16)
const hashes = Object.freeze(["SHA-256", "SHA-384"]);
const defaultMaximum = 8;

// the store key of a host: the first four bytes of its SHA-256, as hex
function keyOf(host)
{
	const digest = new Digest({algorithm: "SHA-256"});
	digest.write(ArrayBuffer.fromString(host));
	const hash = new Uint8Array(digest.read(), 0, 4);
	digest.close();
	return hash.toHex();
}

function expired(view, now)
{
	const age = now - view.getFloat64(0);
	return (age < 0) || (age >= (view.getUint32(8) * 1000));
}

// the ticket to offer for this session's host: {hash, ticket, psk, obfuscatedAge}, or undefined
export function load(session)
{
	const store = session.options.tickets;
	const key = keyOf(session.options.tls_server_name);
	const entry = store.read(key);
	if (!entry)
		return;
	const now = Date.now();
	const view = new DataView(entry);
	if ((entry.byteLength < header) || expired(view, now)) {
		store.delete(key);
		return;
	}
	const hash = hashes[view.getUint8(16)];
	const ticketLength = view.getUint16(17);
	if (!hash || (entry.byteLength !== (header + ticketLength + hashSize(hash)))) {
		store.delete(key);
		return;
	}
	return {
		hash,
		ticket: entry.slice(header, header + ticketLength),
		psk: entry.slice(header + ticketLength),
		obfuscatedAge: ((now - view.getFloat64(0)) + view.getUint32(12)) >>> 0,		// milliseconds, plus ticket_age_add, modulo 2^32
	};
}

// a NewSessionTicket: derive its PSK, store the entry, sweep the domain
export function store(session, lifetime, ageAdd, nonce, ticket)
{
	if (!lifetime)		// zero: the ticket is not to be kept (section 4.6.1)
		return;
	if (lifetime > maximumLifetime)
		lifetime = maximumLifetime;
	const state = session.tls13;
	const tickets = session.options.tickets;
	const hash = state.suite.hash;
	const size = hashSize(hash);
	const psk = hkdfExpandLabel(hash, state.resumptionMasterSecret, "resumption", nonce, size);
	const entry = new Uint8Array(header + ticket.byteLength + size);
	const view = new DataView(entry.buffer);
	view.setFloat64(0, Date.now());
	view.setUint32(8, lifetime);
	view.setUint32(12, ageAdd);
	view.setUint8(16, hashes.indexOf(hash));
	view.setUint16(17, ticket.byteLength);
	entry.set(new Uint8Array(ticket), header);
	entry.set(new Uint8Array(psk), header + ticket.byteLength);
	const key = keyOf(session.options.tls_server_name);
	tickets.write(key, entry);
	sweep(tickets, key, session.options.ticketsMax ?? defaultMaximum);
}

// drop expired entries, then the least recently written until the domain holds at most maximum (the entry under keep among them)
function sweep(tickets, keep, maximum)
{
	const keys = [];
	for (const key of tickets)
		keys.push(key);
	const now = Date.now();
	const live = [], received = [];
	for (let i = 0; i < keys.length; i++) {
		const key = keys[i];
		if (key === keep)
			continue;
		const entry = tickets.read(key);
		if (!entry || (entry.byteLength < header)) {
			tickets.delete(key);
			continue;
		}
		const view = new DataView(entry);
		if (expired(view, now)) {
			tickets.delete(key);
			continue;
		}
		live.push(key);
		received.push(view.getFloat64(0));
	}
	while (live.length >= maximum) {
		let oldest = 0;
		for (let i = 1; i < live.length; i++) {
			if (received[i] < received[oldest])
				oldest = i;
		}
		tickets.delete(live[oldest]);
		live.splice(oldest, 1);
		received.splice(oldest, 1);
	}
}
