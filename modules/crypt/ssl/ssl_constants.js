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

// key exchange algorithms
export const RSA = 0;
export const DHE_DSS = 1;
export const DHE_RSA = 2;
export const DH_ANON = 3;
export const DH_DSS = 4;
export const DH_RSA = 5;
export const ECDHE_RSA = 6;
// encryption algroithms
export const AES = 0;
export const DES = 1;
export const TDES = 2;
export const RC4 = 3;
// hash algorithms
export const SHA1 = 0;
export const MD5 = 1;
export const SHA256 = 2;
export const SHA384 = 3;
export const NULL = 255;
// certificate type
export const CERT_RSA = 0;
export const CERT_DSA = 1;
// encryption mode
export const NONE = 0;
export const CBC = 1;
export const GCM = 2;

export const protocolVersion = (3 << 8) | 1;	// default protocol version
export const minProtocolVersion = (3 << 8) | 1;
export const maxProtocolVersion = (3 << 8) | 3;

export const supportedCompressionMethods = Object.freeze([0]);	// NULL
