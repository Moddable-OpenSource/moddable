/*
 * Copyright (c) 2016-2020  Moddable Tech, Inc.
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

#include "xs.h"
#include "xsPlatform.h"

void setupDebugger() { }
void ESP_putc(int c) { }
int ESP_getc(void) { return -1; }

#ifdef mxDebug

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

void modLog_transmit(const char *msg)
{ 
	static char _msgBuffer[128];
	static uint8_t nrfLogEnabled = 0;
	
	if (0 == nrfLogEnabled) {
		ret_code_t err_code;
		err_code = NRF_LOG_INIT(NULL);
		APP_ERROR_CHECK(err_code);
		NRF_LOG_DEFAULT_BACKENDS_INIT();
		nrfLogEnabled = 1;
	}
	
	uint16_t msgLength = c_strlen(msg);
	if (msgLength < sizeof(_msgBuffer)) {
		c_memcpy(_msgBuffer, msg, msgLength + 1);
		NRF_LOG_RAW_INFO("<mod> %s\r\n", _msgBuffer);
	}
}

#endif
