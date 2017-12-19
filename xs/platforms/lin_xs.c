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

#include "xsAll.h"
#include "xsScript.h"

#include <fcntl.h>
#include <glib.h>
#include <netdb.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>

void fxCreateMachinePlatform(txMachine* the)
{
}

void fxDeleteMachinePlatform(txMachine* the)
{
}

gboolean fxQueuePromiseJobsCallback(void *it)
{
	txMachine* the = it;
	fxRunPromiseJobs(the);
	return G_SOURCE_REMOVE;
}

void fxQueuePromiseJobs(txMachine* the)
{
	GSource* idle_source = g_idle_source_new();
	g_source_set_callback(idle_source, fxQueuePromiseJobsCallback, the, NULL);
	g_source_set_priority(idle_source, G_PRIORITY_DEFAULT);
	g_source_attach(idle_source, g_main_context_get_thread_default());
	g_source_unref(idle_source);
}

#ifdef mxDebug
extern char *program_invocation_name;

gboolean fxReadableCallback(GSocket *socket, GIOCondition condition, gpointer user_data)
{
	txMachine* the = user_data;
	if (fxIsReadable(the)) {
		fxDebugCommand(the);
		if (the->breakOnStartFlag) {
			fxBeginHost(the);
			fxDebugLoop(the, NULL, 0, "C: xsDebugger");
			fxEndHost(the);
		}
	}
	return TRUE;
}

void fxConnect(txMachine* the)
{
	char name[256];
	char* colon;
	int port;
	struct sockaddr_in address;
	int fd = -1;
	int	flag;
	colon = getenv("XSBUG_HOST");
	if ((colon) && (c_strlen(colon) + 1 < sizeof(name))) {
		c_strcpy(name, colon);
		colon = strchr(name, ':');
		if (colon == NULL)
			port = 5002;
		else {
			*colon = 0;
			colon++;
			port = strtol(colon, NULL, 10);
		}
	}
	else {
		if (strstr(program_invocation_name, "xsbug"))
			port = 5003;
		else
			port = 5002;
		strcpy(name, "localhost");
	}
	memset(&address, 0, sizeof(address));
  	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr(name);
	if (address.sin_addr.s_addr == INADDR_NONE) {
		struct hostent *host = gethostbyname(name);
		if (!host)
			return;
		memcpy(&(address.sin_addr), host->h_addr, host->h_length);
	}
  	address.sin_port = htons(port);
	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
		return;
	signal(SIGPIPE, SIG_IGN);
	flag = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flag | O_NONBLOCK);
	if (connect(fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
    	 if (errno == EINPROGRESS) { 
			fd_set fds;
			struct timeval timeout = { 2, 0 }; // 2 seconds, 0 micro-seconds
			int error = 0;
			unsigned int length = sizeof(error);
			FD_ZERO(&fds);
			FD_SET(fd, &fds);
			if (select(fd + 1, NULL, &fds, NULL, &timeout) == 0)
				goto bail;
			if (!FD_ISSET(fd, &fds))
				goto bail;
			if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &length) < 0)
				goto bail;
			if (error)
				goto bail;
		}
		else
			goto bail;
	}
	fcntl(fd, F_SETFL, flag);
	signal(SIGPIPE, SIG_DFL);
	
	the->socket = g_socket_new_from_fd(fd, NULL);
	if (!the->socket)
		goto bail;
		
	g_socket_set_blocking(the->socket, FALSE);
	the->source = g_socket_create_source(the->socket, G_IO_IN, NULL);
	g_source_set_callback(the->source, (void*)fxReadableCallback, the, NULL);
	g_source_set_priority(the->source, G_PRIORITY_DEFAULT);
	g_source_attach(the->source, g_main_context_get_thread_default());
	
	return;
bail:
	if (fd >= 0)
		close(fd);
}

void fxDisconnect(txMachine* the)
{
	if (the->source) {
		g_source_destroy(the->source);
		the->source = NULL;
	}
	if (the->socket) {
		g_socket_close(the->socket, NULL);
		g_object_unref(the->socket);
		the->socket = NULL;
	}
}

txBoolean fxIsConnected(txMachine* the)
{
	return (the->socket != NULL) ? 1 : 0;
}

txBoolean fxIsReadable(txMachine* the)
{
	if (the->socket != NULL) {
		gssize count = g_socket_receive(the->socket, the->debugBuffer, sizeof(the->debugBuffer) - 1, NULL, NULL);
		if (count > 0) {
			the->debugOffset = count;
			return 1;
		}
	}
	return 0;
}

void fxReceive(txMachine* the)
{
	if (the->socket != NULL) {
		GError* error = NULL;
		gssize count;
	again:
		count = g_socket_receive(the->socket, the->debugBuffer + the->debugOffset, sizeof(the->debugBuffer) - the->debugOffset - 1, NULL, &error);
		if (count < 0) {
			if (error->code == G_IO_ERROR_WOULD_BLOCK) {
				g_clear_error(&error);
				if (the->debugOffset == 0)
					goto again;
			}
			else
				fxDisconnect(the);
		}
		the->debugOffset = count;
		the->debugBuffer[the->debugOffset] = 0;
	}
}

void fxSend(txMachine* the, txBoolean more)
{
	if (the->socket != NULL) {
		GError* error = NULL;
		gssize count;
	again:
		count = g_socket_send(the->socket, the->echoBuffer, the->echoOffset, NULL, &error); 
		if (count < 0) {
			if (error->code == G_IO_ERROR_WOULD_BLOCK) {
				g_clear_error(&error);
				goto again;
			}
			fxDisconnect(the);
		}
	}
}
#endif
