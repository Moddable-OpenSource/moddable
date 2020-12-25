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

/*
	to do:

		string;ascii doesn't ensure data is valid

*/

#include "xsAll.h"
#include "mc.xs.h"

#include <glib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/ioctl.h>

#include <termios.h>
#define BOTHER 0010000
struct termios2 {
	tcflag_t c_iflag;
	tcflag_t c_oflag;
	tcflag_t c_cflag;
	tcflag_t c_lflag;
	cc_t c_line;
	cc_t c_cc[19];
	speed_t c_ispeed;
	speed_t c_ospeed;
};
#define TCGETS2 _IOR('T', 0x2A, struct termios2)
#define TCSETS2 _IOW('T', 0x2B, struct termios2)

typedef struct {
	xsMachine			*the;
	xsSlot				obj;
	xsSlot				onError;
	xsSlot				onReadable;
	xsSlot				onWritable;
	int					fd;
	GIOChannel*			channel;
	guint				source;
	void*				job;
	uint8_t				bufferFormat;
	uint8_t				hasOnError;
	uint8_t				hasOnReadable;
	uint8_t				hasOnWritable;
} xsSerialRecord, *xsSerial;

typedef struct {
	txWorkerJob* next;
	txWorkerCallback callback;
	xsSerial serial;
} xsSerialJobRecord, *xsSerialJob;

static void xs_serial_format_set_aux(xsMachine *the, xsSerial s, char *format);
static gboolean xs_serial_read_callback(GIOChannel *source, GIOCondition condition, gpointer data);
static void xs_serial_write_aux(void* machine, xsSerial s);
static void xs_serial_write_callback(void* machine, void* it);

#ifdef mxDebug
	#define xsElseThrow(_ASSERTION) \
		((void)((_ASSERTION) || (fxThrowMessage(the,(char *)__FILE__,__LINE__,XS_UNKNOWN_ERROR,strerror(errno)), 1)))
#else
	#define xsElseThrow(_ASSERTION) \
		((void)((_ASSERTION) || (fxThrowMessage(the,NULL,0,XS_UNKNOWN_ERROR,strerror(errno)), 1)))
#endif

void xs_serial_destructor(void *data)
{
	xsSerial s = data;
	if (!s) return;

	if (s->source)
		g_source_remove(s->source);
	if (s->channel) {
		g_io_channel_shutdown(s->channel, TRUE, NULL);
		g_io_channel_unref(s->channel);
	}
	if (s->fd != -1)
		close(s->fd);
	if (s->job)
		((xsSerialJob)s->job)->serial = NULL;
	free(data);
}

void xs_serial_constructor(xsMachine *the)
{
	xsSerial s = NULL;
	xsIntegerValue baud;
	xsStringValue device;
	struct termios tio;
	struct termios2 tio2;
  	GError *error = NULL;
	xsVars(1);
	xsTry {
		s = calloc(1, sizeof(xsSerialRecord));
		xsElseThrow(s != NULL);
		s->the = the;
		s->obj = xsThis;
		s->onError = xsGet(xsArg(0), xsID_onError);
		s->hasOnError = xsTest(s->onError);
		s->onReadable = xsGet(xsArg(0), xsID_onReadable);
		s->hasOnReadable = xsTest(s->onReadable);
		s->onWritable = xsGet(xsArg(0), xsID_onWritable);
		s->hasOnWritable = xsTest(s->onWritable);
		if (xsHas(xsArg(0), xsID_format)) {
			xsVar(0) = xsGet(xsArg(0), xsID_format);
			xs_serial_format_set_aux(the, s, xsToString(xsVar(0)));
		}
		else
			s->bufferFormat = 1;
		if (xsHas(xsArg(0), xsID_target)) {
			xsVar(0) = xsGet(xsArg(0), xsID_target);
			xsSet(xsThis, xsID_target, xsVar(0));
		}
		xsVar(0) = xsGet(xsArg(0), xsID_baud);
		baud = xsToInteger(xsVar(0));
		xsVar(0) = xsGet(xsArg(0), xsID_device);
		device = xsToString(xsVar(0));

		s->fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
		xsElseThrow(s->fd != -1);
		c_memset(&tio, 0, sizeof(tio));
		tio.c_cflag = CS8 | CLOCAL | CREAD;
		tio.c_iflag = IGNPAR;
		tio.c_cc[VMIN] = 1;
		xsElseThrow(tcsetattr(s->fd, TCSANOW, &tio) == 0);
		xsElseThrow(ioctl(s->fd, TCGETS2, &tio2) == 0);
		tio2.c_cflag &= ~CBAUD;
		tio2.c_cflag |= BOTHER;
		tio2.c_ispeed = baud;
		tio2.c_ospeed = baud;
		xsElseThrow(ioctl(s->fd, TCSETS2, &tio2) == 0);
		
		s->channel = g_io_channel_unix_new(s->fd);
		xsElseThrow(s->channel != NULL);
		xsElseThrow(g_io_channel_set_encoding(s->channel, NULL, &error) == G_IO_STATUS_NORMAL);
		s->source = g_io_add_watch(s->channel, G_IO_IN | G_IO_ERR, xs_serial_read_callback, s);
		xsElseThrow(s->source != 0);
		if (s->hasOnWritable)
			xs_serial_write_aux(the, s);
	}
	xsCatch {
		xs_serial_destructor(s);
		xsThrow(xsException);
	}	
	xsSetHostData(xsThis, s);
	xsRemember(s->obj);
	if (s->hasOnError)
		xsRemember(s->onError);
	if (s->hasOnReadable)
		xsRemember(s->onReadable);
	if (s->hasOnWritable)
		xsRemember(s->onWritable);
}

void xs_serial_close(xsMachine *the)
{
	xsSerial s = xsGetHostData(xsThis);
	if (!s) return;

	xsForget(s->obj);
	if (s->hasOnError)
		xsForget(s->onError);
	if (s->hasOnReadable)
		xsForget(s->onReadable);
	if (s->hasOnWritable)
		xsForget(s->onWritable);
	xs_serial_destructor(s);
	
	xsSetHostData(xsThis, NULL);
}

void xs_serial_read(xsMachine *the)
{
	xsSerial s = xsGetHostData(xsThis);
	int available = 0;
	int count;

	ioctl(s->fd, FIONREAD, &available);
	if (0 == available)
		return;

	if (xsToInteger(xsArgc)) {
		count = xsToInteger(xsArg(0));
		if (count > available)
			count = available;
	}
	else
		count = available;

	if (s->bufferFormat) {
		xsResult = xsArrayBuffer(NULL, count);
		read(s->fd, xsToArrayBuffer(xsResult), count);
	}
	else {
		xsResult = xsStringBuffer(NULL, count);
		read(s->fd, xsToString(xsResult), count);
	}
}

gboolean xs_serial_read_callback(GIOChannel *source, GIOCondition condition, gpointer data)
{
	xsSerial s = data;
	if (condition & G_IO_ERR) {
		xsBeginHost(s->the);
		if (s->hasOnError) {
			xsTry {
				xsCallFunction0(s->onError, s->obj);
			}
			xsCatch {
			}
		}
		xsCall0(s->obj, xsID("close"));
		xsEndHost();
	}
	else if (condition & G_IO_IN) {
		if (s->hasOnReadable) {
			xsBeginHost(s->the);
			xsTry {
				xsCallFunction1(s->onReadable, s->obj, xsInteger(1));
			}
			xsCatch {
			}
			xsEndHost();
		}
	}
	return TRUE;
}

void xs_serial_purge(xsMachine *the)
{
}

void xs_serial_set(xsMachine *the)
{
	xsSerial s = xsGetHostData(xsThis);
	int flags;

	ioctl(s->fd, TIOCMGET, &flags);

	if (xsHas(xsArg(0), xsID_RTS)) {
		xsResult = xsGet(xsArg(0), xsID_RTS);
		if (xsTest(xsResult))
			flags |= TIOCM_RTS;
		else
			flags &= ~TIOCM_RTS;
	}

	if (xsHas(xsArg(0), xsID_DTR)) {
		xsResult = xsGet(xsArg(0), xsID_DTR);
		if (xsTest(xsResult))
			flags |= TIOCM_DTR;
		else
			flags &= ~TIOCM_DTR;
	}

	ioctl(s->fd, TIOCMSET, &flags);
}

void xs_serial_write(xsMachine *the)
{
	xsSerial s = xsGetHostData(xsThis);
	char *data;
	int count;

	if (s->bufferFormat) {
		data = xsToArrayBuffer(xsArg(0));
		count = xsGetArrayBufferLength(xsArg(0));
	}
	else {
		data = xsToString(xsArg(0));
		count = strlen(data);
	}
	if (!count)
		return;

	while (count) {
		int result = write(s->fd, data, count);
		xsElseThrow(result >= 0);
		count -= result;
		data += result;
	}
	
	if (!s->job && s->hasOnWritable) {
		xs_serial_write_aux(the, s);
	}
}

void xs_serial_write_aux(void* the, xsSerial s)
{
	xsSerialJob job = c_calloc(sizeof(xsSerialJobRecord), 1);
	xsElseThrow(job != NULL);
	job->callback = xs_serial_write_callback;
	job->serial = s;
	fxQueueWorkerJob(the, job);
	s->job = job;
}

void xs_serial_write_callback(void* machine, void* it)
{
	xsSerialJob job = it;
	xsSerial s = job->serial;
	if (s) {
		s->job = NULL;
		xsBeginHost(machine);
		xsTry {
			xsCallFunction1(s->onWritable, s->obj, xsInteger(1));
		}
		xsCatch {
		}
		xsEndHost();
	}
}

void xs_serial_format_get(xsMachine *the)
{
	xsSerial s = xsGetHostData(xsThis);
	if (s->bufferFormat)
		xsResult = xsString("buffer");
	else
		xsResult = xsString("string;ascii");
}

void xs_serial_format_set(xsMachine *the)
{
	xsSerial s = xsGetHostData(xsThis);
	xs_serial_format_set_aux(the, s, xsToString(xsArg(0)));
}

void xs_serial_format_set_aux(xsMachine *the, xsSerial s, char *format)
{
	if (!strcmp(format, "buffer"))
		s->bufferFormat = 1;
	else if (!strcmp(format, "string;ascii"))
		s->bufferFormat = 0;
	else
		xsUnknownError("invalid format");
}


