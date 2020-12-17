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
	xsSlot				onReadable;
	xsSlot				onWritable;
	xsSlot				onError;

	int					fd;
	GIOChannel*			channel;
	guint				source;
	void*				job;
	uint8_t				bufferFormat;
	uint8_t				hasOnReadable;
	uint8_t				hasOnWritable;
	uint8_t				hasOnError;
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

void xs_serial_destructor(void *data)
{
	xsSerial s = data;
	if (!s) return;

	if (s->job)
		((xsSerialJob)s->job)->serial = NULL;
	if (s->source)
		g_source_remove(s->source);
	if (s->channel) {
		g_io_channel_shutdown(s->channel, TRUE, NULL);
		g_io_channel_unref(s->channel);
	}
	else if (s->fd >= 0)
		close(s->fd);
	free(data);
}

void xs_serial_constructor(xsMachine *the)
{
	char device[128], format[64];
	xsIntegerValue baud;
	int fd;
	struct termios tio;
	struct termios2 tio2;

	xsSerial s;
	xsSlot onReadable, onWritable, onError;
  	GError *error=NULL;

	xsVars(2);

	xsVar(0) = xsGet(xsArg(0), xsID_device);
	xsToStringBuffer(xsVar(0), device, sizeof(device));

	xsVar(0) = xsGet(xsArg(0), xsID_baud);
	baud = xsToInteger(xsVar(0));

	onReadable = xsGet(xsArg(0), xsID_onReadable);
	onWritable = xsGet(xsArg(0), xsID_onWritable);
	onError = xsGet(xsArg(0), xsID_onError);

	if (xsHas(xsArg(0), xsID_format)) {
		xsVar(0) = xsGet(xsArg(0), xsID_format);
		xsToStringBuffer(xsVar(0), format, sizeof(format));
	}
	else
		format[0] = 0;

	if (xsHas(xsArg(0), xsID_target)) {
		xsVar(0) = xsGet(xsArg(0), xsID_target);
		xsSet(xsThis, xsID_target, xsVar(0));
	}

	s = calloc(1, sizeof(xsSerialRecord));
	if (!s)
		xsUnknownError("not enough memory");
	xsSetHostData(xsThis, s);
	s->the = the;
	s->obj = xsThis;
	s->hasOnWritable = xsTest(onWritable);
	s->hasOnReadable = xsTest(onReadable);
	s->hasOnError = xsTest(onError);

	if (format[0])
		xs_serial_format_set_aux(the, s, format);
	else
		s->bufferFormat = 1;

	fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (-1 == fd)
		xsUnknownError(strerror(errno));
	c_memset(&tio, 0, sizeof(tio));
	tio.c_cflag = CS8 | CLOCAL | CREAD;
	tio.c_iflag = IGNPAR;
	tio.c_cc[VMIN] = 1;
	if (tcsetattr(fd, TCSANOW, &tio) != 0)
		xsUnknownError("can't configure serial");
// 	tcflush(fd, TCIFLUSH);
	ioctl(fd, TCGETS2, &tio2);
	tio2.c_cflag &= ~CBAUD;
	tio2.c_cflag |= BOTHER;
	tio2.c_ispeed = baud;
	tio2.c_ospeed = baud;
	ioctl(fd, TCSETS2, &tio2);

	s->fd = fd;
	s->channel = g_io_channel_unix_new(fd);
  	g_io_channel_set_encoding(s->channel, NULL, &error); // raw data, no encoding
	s->source = g_io_add_watch(s->channel, G_IO_IN | G_IO_ERR, xs_serial_read_callback, s);

	xsRemember(s->obj);

	if (s->hasOnReadable) {
		s->onReadable = onReadable;
		xsRemember(s->onReadable);
	}
	if (s->hasOnError) {
		s->onError = onError;
		xsRemember(s->onError);
	}
	if (s->hasOnWritable) {
		s->onWritable = onWritable;
		xsRemember(s->onWritable);
		xs_serial_write_aux(the, s);
	}
}

void xs_serial_close(xsMachine *the)
{
	xsSerial s = xsGetHostData(xsThis);
	if (!s) return;

	xsForget(s->obj);
	if (s->hasOnReadable)
		xsForget(s->onReadable);
	if (s->hasOnWritable)
		xsForget(s->onWritable);
	if (s->hasOnError)
		xsForget(s->onError);
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
			int count;
			xsBeginHost(s->the);
				ioctl(s->fd, FIONREAD, &count);
				xsCallFunction1(s->onReadable, s->obj, xsInteger(count));
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
	int count;
	char *data;

	if (s->bufferFormat) {
		count = xsGetArrayBufferLength(xsArg(0));
		data = xsToArrayBuffer(xsArg(0));
	}
	else {
		data = xsToString(xsArg(0));
		count = strlen(data);
	}
	if (!count)
		return;

	while (count) {
		int result = write(s->fd, data, count);
		if (result < 0)
			xsUnknownError("write failed");
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
	if (job == NULL)
		xsUnknownError("not enough memory");
	job->callback = xs_serial_write_callback;
	job->serial = s;
	s->job = job;
	fxQueueWorkerJob(the, job);
}

void xs_serial_write_callback(void* machine, void* it)
{
	xsSerialJob job = it;
	xsSerial s = job->serial;
	if (s) {
		s->job = NULL;
		xsBeginHost(machine);
			xsCallFunction1(s->onWritable, s->obj, xsInteger(1024));		// 1024 is the default on Linux?
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


