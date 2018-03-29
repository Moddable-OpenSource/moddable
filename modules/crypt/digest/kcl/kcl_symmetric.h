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

#ifndef __KCL_SYMMETRIC_H__
#define __KCL_SYMMETRIC_H__
 
#include "kcl.h"
#include <stdint.h>	/* for uint64_t */
#include <stddef.h>	/* for size_t */

typedef enum {
	KCL_DIRECTION_ENCRYPTION,
	KCL_DIRECTION_DECRYPTION,
} kcl_symmetric_direction_t;

extern kcl_err_t kcl_des_init(void **ctxp, const void *key, size_t keysize);
extern void kcl_des_keysched(void *ctx, kcl_symmetric_direction_t direction);
extern void kcl_des_process(void *ctx, const void *in, void *out);
extern void kcl_des_size(void *ctx, size_t *blksz, size_t *keysz);
extern void kcl_des_finish(void *ctx);
extern kcl_err_t kcl_tdes_init(void **ctxp, const void *key, size_t keysize);
extern void kcl_tdes_keysched(void *ctx, kcl_symmetric_direction_t direction);
extern void kcl_tdes_process(void *ctx, const void *in, void *out);
extern void kcl_tdes_size(void *ctx, size_t *blksz, size_t *keysz);
extern void kcl_tdes_finish(void *ctx);
extern kcl_err_t kcl_aes_init(void **ctxp, const void *key, size_t keysize, size_t blocksize);
extern void kcl_aes_keysched(void *ctx, kcl_symmetric_direction_t direction);
extern void kcl_aes_process(void *ctx, const void *in, void *out);
extern void kcl_aes_size(void *ctx, size_t *blksz, size_t *keysz);
extern void kcl_aes_finish(void *ctx);
extern kcl_err_t kcl_rc4_init(void **ctxp, const void *key, size_t keysize);
extern void kcl_rc4_process(void *ctx, const void *in, void *out, size_t n);
extern void kcl_rc4_finish(void *ctx);
extern kcl_err_t kcl_chacha_init(void **ctxp, const void *key, size_t keysize, const void *iv, size_t ivsize, uint64_t counter);
extern void kcl_chacha_process(void *ctx, const void *in, void *out, size_t n);
extern void kcl_chacha_finish(void *ctx);
extern void kcl_chacha_setIV(void *ctx, const void *iv, size_t ivsize, uint64_t counter);

extern kcl_err_t kcl_sha1_create(void **ctxp);
extern void kcl_sha1_init(void *ctx);
extern void kcl_sha1_update(void *ctx, const void *data, size_t sz);
extern void kcl_sha1_result(void *ctx, void *result);
extern void kcl_sha1_finish(void *ctx);
extern void kcl_sha1_size(void *ctx, size_t *blksz, size_t *outsz);
extern kcl_err_t kcl_sha256_create(void **ctxp);
extern void kcl_sha256_init(void *ctx);
extern void kcl_sha256_update(void *ctx, const void *data, size_t sz);
extern void kcl_sha256_result(void *ctx, void *result);
extern void kcl_sha256_finish(void *ctx);
extern void kcl_sha256_size(void *ctx, size_t *blksz, size_t *outsz);
extern kcl_err_t kcl_sha224_create(void **ctxp);
extern void kcl_sha224_init(void *ctx);
extern void kcl_sha224_update(void *ctx, const void *data, size_t sz);
extern void kcl_sha224_result(void *ctx, void *result);
extern void kcl_sha224_finish(void *ctx);
extern void kcl_sha224_size(void *ctx, size_t *blksz, size_t *outsz);
extern kcl_err_t kcl_sha512_create(void **ctxp);
extern void kcl_sha512_init(void *ctx);
extern void kcl_sha512_update(void *ctx, const void *data, size_t sz);
extern void kcl_sha512_result(void *ctx, void *result);
extern void kcl_sha512_finish(void *ctx);
extern void kcl_sha512_size(void *ctx, size_t *blksz, size_t *outsz);
extern kcl_err_t kcl_sha384_create(void **ctxp);
extern void kcl_sha384_init(void *ctx);
extern void kcl_sha384_update(void *ctx, const void *data, size_t sz);
extern void kcl_sha384_result(void *ctx, void *result);
extern void kcl_sha384_finish(void *ctx);
extern void kcl_sha384_size(void *ctx, size_t *blksz, size_t *outsz);
extern kcl_err_t kcl_md5_create(void **ctxp);
extern void kcl_md5_init(void *ctx);
extern void kcl_md5_update(void *ctx, const void *data, size_t sz);
extern void kcl_md5_result(void *ctx, void *result);
extern void kcl_md5_finish(void *ctx);
extern void kcl_md5_size(void *ctx, size_t *blksz, size_t *outsz);

#endif /* __KCL_SYMMETRIC_H__ */
