/*
  i2s.h - Software I2S library for esp8266

  Copyright (c) 2015 Hristo Gochkov. All rights reserved.
  This file is part of the esp8266 core for Arduino environment.
 
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#ifndef I2S_h
#define I2S_h

/*
	 Stripped version of i2s.h from ESP8266 Arduino SDK version 2.3
	 See tinyi2s.c for details.
	 For use with Moddable SDK.
*/

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*I2SRenderBuffer)(void *refcon, int16_t *lr, int count);

void i2s_begin(I2SRenderBuffer render, void *refcon, uint32_t rate);
void i2s_end();

#ifdef __cplusplus
}
#endif

#endif
