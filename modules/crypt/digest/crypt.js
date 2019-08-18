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

export class Digest @ "xs_crypt_Digest_destructor" {
	constructor(type) @ "xs_crypt_Digest";
	write(data) @ "xs_crypt_Digest_write";
	close(more) @ "xs_crypt_Digest_close";
	reset() @ "xs_crypt_Digest_reset";
	get blockSize() @ "xs_crypt_Digest_get_blockSize";
	get outputSize() @ "xs_crypt_Digest_get_outputSize";
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
	_init(h, aad) @ "xs_ghash_init";
};

export class BlockCipher @ "xs_crypt_cipher_destructor" {
	constructor(cipher, key) @ "xs_crypt_cipher_constructor";
	encrypt(data, result) @ "xs_crypt_cipher_encrypt";
	decrypt(data, result) @ "xs_crypt_cipher_decrypt";
	get keySize() @ "xs_crypt_cipher_get_keySize";
	get blockSize() @ "xs_crypt_cipher_get_blockSize";
};

export class StreamCipher @ "xs_crypt_streamcipher_destructor" {
	constructor(cipher, key) @ "xs_crypt_streamcipher_constructor";
	encrypt(data, result, count) @ "xs_crypt_streamcipher_process";
	decrypt(data, result, count) @ "xs_crypt_streamcipher_process";
	setIV(iv, counter) @ "xs_crypt_streamcipher_setIV";
};

export class Mode @ "xs_crypt_mode_delete" {
	constructor(mode, cipher, iv, pad) @ "xs_crypt_mode_constructor";
	encrypt(data, result, count) @ "xs_crypt_mode_encrypt";
	decrypt(data, result, count) @ "xs_crypt_mode_decrypt";
	setIV(iv) @ "xs_crypt_mode_setIV";
	get eof() @ "xs_crypt_mode_get_eof";
	set eof() @ "xs_crypt_mode_set_eof";
};

export default Object.freeze({Digest, BlockCipher, Mode, StreamCipher, GHASH});
