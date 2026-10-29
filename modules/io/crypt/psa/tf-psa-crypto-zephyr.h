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
	TF-PSA-Crypto settings for the PSA backend on Zephyr, included after Zephyr's own configuration
	(modules/mbedtls/configs/config-tf-psa-crypto.h), which sizes for least RAM.
*/

// a key store that grows with the keys in use, as ESP-IDF builds it, rather than Zephyr's fixed table
// of CONFIG_MBEDTLS_PSA_KEY_SLOT_COUNT: each Cipher, Signature and KeyAgreement holds a key until closed
#define MBEDTLS_PSA_KEY_STORE_DYNAMIC

// speed over a little RAM: Zephyr's 2 and 0 make EC operations on the NIST curves 5 to 10 times slower
#undef MBEDTLS_ECP_WINDOW_SIZE
#define MBEDTLS_ECP_WINDOW_SIZE 4
#undef MBEDTLS_ECP_FIXED_POINT_OPTIM
#define MBEDTLS_ECP_FIXED_POINT_OPTIM 1
#undef MBEDTLS_MPI_WINDOW_SIZE
#define MBEDTLS_MPI_WINDOW_SIZE 6
