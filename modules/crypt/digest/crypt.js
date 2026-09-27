/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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
 * This file incorporates work covered by the following copyright and  
 * permission notice:  
 *
 *       Copyright (C) 2010-2016 Marvell International Ltd.
 *       Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *       Licensed under the Apache License, Version 2.0 (the "License");
 *       you may not use this file except in compliance with the License.
 *       You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *       Unless required by applicable law or agreed to in writing, software
 *       distributed under the License is distributed on an "AS IS" BASIS,
 *       WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *       See the License for the specific language governing permissions and
 *       limitations under the License.
 */

/*
	crypt
*/

export class Digest extends Native("xs_crypt_Digest_destructor") {
	constructor(type) { super(); native("xs_crypt_Digest").call(this, type); }
	write(data) { return native("xs_crypt_Digest_write").call(this, data); }
	close(more) { return native("xs_crypt_Digest_close").call(this, more); }
	reset() { return native("xs_crypt_Digest_reset").call(this); }
	get blockSize() { return native("xs_crypt_Digest_get_blockSize").call(this); }
	get outputSize() { return native("xs_crypt_Digest_get_outputSize").call(this); }
	process() {
		this.reset();
		for (let i = 0; i < arguments.length; i++)
			this.write(arguments[i]);
		return this.close();
	}
	update(data) {
		this.write(data);
	}
};

export class GHASH extends Digest {
	constructor(h, aad) {
		super("GHASH");
		this._init(h, aad);
	};
	_init(h, aad) { return native("xs_ghash_init").call(this, h, aad); }
};

export class BlockCipher extends Native("xs_crypt_cipher_destructor") {
	constructor(cipher, key) { super(); native("xs_crypt_cipher_constructor").call(this, cipher, key); }
	encrypt(data, result) { return native("xs_crypt_cipher_encrypt").call(this, data, result); }
	decrypt(data, result) { return native("xs_crypt_cipher_decrypt").call(this, data, result); }
	get keySize() { return native("xs_crypt_cipher_get_keySize").call(this); }
	get blockSize() { return native("xs_crypt_cipher_get_blockSize").call(this); }
};

export class StreamCipher extends Native("xs_crypt_streamcipher_destructor") {
	constructor(cipher, key, iv, counter) { super(); native("xs_crypt_streamcipher_constructor").call(this, cipher, key, iv, counter); }
	encrypt(data, result, count) { return native("xs_crypt_streamcipher_process").call(this, data, result, count); }
	decrypt(data, result, count) { return native("xs_crypt_streamcipher_process").call(this, data, result, count); }
	setIV(iv, counter) { return native("xs_crypt_streamcipher_setIV").call(this, iv, counter); }
};

export class Mode extends Native("xs_crypt_mode_delete") {
	constructor(mode, cipher, iv, pad) { super(); native("xs_crypt_mode_constructor").call(this, mode, cipher, iv, pad); }
	encrypt(data, result, count) { return native("xs_crypt_mode_encrypt").call(this, data, result, count); }
	decrypt(data, result, count) { return native("xs_crypt_mode_decrypt").call(this, data, result, count); }
	setIV(iv) { return native("xs_crypt_mode_setIV").call(this, iv); }
	get eof() { return native("xs_crypt_mode_get_eof").call(this); }
	set eof(it) { native("xs_crypt_mode_set_eof").call(this, it); }
};

export default Object.freeze({Digest, BlockCipher, Mode, StreamCipher, GHASH});
