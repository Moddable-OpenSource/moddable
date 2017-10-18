/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Tools.
 * 
 *   The Moddable SDK Tools is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Tools is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 * 
 *   You should have received a copy of the GNU General Public License
 *   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>
#include <poll.h>
#include <errno.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

static struct termios tio_orig;
static int serial_fd;
static char* ser_path;
static speed_t ser_speed;
static int ser_data;
static int ser_parity;
static int ser_stop;

static int server_fd;
static char* server;
static int port;


static int socket_fd;

#ifdef DEBUG
#define DBG(m, ...) fprintf(stderr, m, ##__VA_ARGS__)
#else
#define DBG(...)
#endif

#define WHY_COPACETIC 0
#define WHY_SIGNAL 1
#define WHY_COMMANDLINE 2
#define WHY_SERIALPATH 3
#define WHY_SERVER 4
#define WHY_POLL_ERROR 5
#define WHY_SERIAL_GONE 6
#define WHY_XSBUG_HOST 7
#define WHY_SERIAL 8
#define WHY_INTERNAL_ERROR 9

#define BOTHER 0010000
#define BUF_SIZE 1024

void die(int why) {
  if (serial_fd > 0) {
    tcsetattr(serial_fd, TCSANOW, &tio_orig);
    close(serial_fd);
  }
  if (socket_fd > 0) {
    close(socket_fd);
  }
  exit(why);
}

void sig_handler(int sig) {
  fprintf(stderr, "\nShutting down on signal\n");
  die(WHY_SIGNAL);
}

void usage(char* name) {
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "%s <device> <speed> <config> [host] [port]\n\n", name);
  fprintf(stderr, "<device> is a file, e.g. /dev/ttyUSB0\n");
  fprintf(stderr, "<baud> is a number, e.g. 115200\n");
  fprintf(stderr, "<config> is a serial port config spec, e.g. 8N1\n");
  fprintf(stderr, "[host] is the IP or host name of an xsbug server; default 127.0.0.1\n");
  fprintf(stderr, "[port] is the port of the xsbug server; default 5002\n\n");
  fprintf(stderr, "[host] and [port] must be used together or not at all.\n\n");
}

void init_args(int argc, char** argv) {
  // must have at least 3 args
  if (argc < 4) {
    usage(argv[0]);
    die(WHY_COMMANDLINE);
  }

  // optional args must appear together
  if (argc == 5 || argc > 6) {
    usage(argv[0]);
    die(WHY_COMMANDLINE);
  }

  ser_path = argv[1];

  // check conformity of port speed
  errno = 0;
  ser_speed = strtol(argv[2], NULL, 10);
  if (errno || ser_speed <= 0) {
    usage(argv[0]);
    if (errno) perror(argv[0]);
    die(WHY_COMMANDLINE);
  }

  // break down the 3-char port config, e.g. "8N1"
  if (strlen(argv[3]) != 3) {
    usage(argv[0]);
    die(WHY_COMMANDLINE);
  }
  ser_data = argv[3][0] - '0';
  ser_parity = argv[3][1];
  ser_stop = argv[3][2] - '0';
  if (ser_data <= 0 || ser_data > 8 || ser_stop <= 0 || ser_stop > 2) {
    usage(argv[0]);
    die(WHY_COMMANDLINE);
  }
  if (ser_parity != 'N' && ser_parity != 'O' && ser_parity != 'E') {
    usage(argv[0]);
    die(WHY_COMMANDLINE);
  }

  if (argc < 5) return;

  // store server, make sure it's not null string
  if (!argv[4]) {
    usage(argv[0]);
    die(WHY_COMMANDLINE);
  }
  server = argv[4];

  // check conformity of port
  errno = 0;
  port = strtol(argv[5], NULL, 10);
  if (errno || port <= 0) {
    usage(argv[0]);
    if (errno) perror(argv[0]);
    die(WHY_COMMANDLINE);
  }
}

// to set non-predefined port speeds on Linux, you need to use ioctl(). These blocks and
// set_user_speed() below inspired by https://github.com/konkers/sconsole/blob/master/sconsole.c
// reproduce the serial driver ioctl config so we can make calls to it
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

void set_user_speed(int fd, speed_t speed) {
  struct termios t;
  struct termios2 t2;
  speed_t sval;

  // speed came in as plain int; convert it to one of the #include constant values
  switch (speed) {
    case 115200:
      sval = B115200; break;
    case 57600:
      sval = B57600; break;
    case 38400:
      sval = B38400; break;
    case 19200:
      sval = B19200; break;
    case 9600:
      sval = B9600; break;
    default:
      sval = BOTHER; break;
  }

  if (sval == BOTHER) {
    // if user specified custom speed, try to set it using Linux specific ioctl
    ioctl(fd, TCGETS2, &t2);
    t2.c_cflag = BOTHER | CS8 | CLOCAL | CREAD;
    t2.c_ispeed = speed;
    t2.c_ospeed = speed;
  } else {
    if (tcgetattr(fd, &t)) {
      perror("warning: failed preparing serial port speed");
    }
    t.c_cflag = sval | CS8 | CLOCAL | CREAD;
    t.c_ispeed = sval;
    t.c_ospeed = sval;
    if (tcsetattr(fd, TCSANOW, &t)) {
      perror("failed setting serial port speed");
      die(WHY_SERIAL);
    }
  }
}

int open_serial(char* path, speed_t speed) {
  struct termios tio;
  int fd;

  // attempt to get an fd
  fd = open(path, O_RDWR | O_NOCTTY | O_NDELAY);
  if (fd < 0) {
    perror("unable to open serial port");
    die(WHY_SERIAL);
  }

  // save current termios port config for fd
  if (tcgetattr(fd, &tio_orig)) {
    perror("warning: unable to save serial settings");
  }

  // set default serial port properties
  tio.c_cflag = B115200 | CS8 | CLOCAL | CREAD;
  tio.c_ispeed = B115200;
  tio.c_ospeed = B115200;
  tio.c_iflag = IGNPAR;
  tio.c_oflag &= ~ONLCR;
  tio.c_lflag = 0; /* turn off CANON, ECHO*, etc */
  tio.c_cc[VTIME] = 0;
  tio.c_cc[VMIN] = 1;
  tcsetattr(fd, TCSANOW, &tio);
  tcflush(fd, TCIFLUSH);

  // then apply the user's desired speed
  set_user_speed(fd, speed);

  return fd;
}

// copy count bytes of src to fd, looping til complete
void copy_to(int fd, const uint8_t *src, size_t count) {
  size_t wcount;
  while (count) {
    wcount = write(fd, src, count);
    #ifdef DEBUG
    printf("\n******************** to %d\n", fd);
    write(0, src, count);
    printf("\n********************\n");
    #endif
    if (wcount < 1) {
      if (wcount < 0) {
        perror("fd disconnected");
      } else {
        fprintf(stderr, "failure writing to fd");
      }
      die(WHY_SERIAL_GONE);
    }
    count -= wcount;
    src += wcount;
  }
}

int open_server(char* host, int port) {
  struct addrinfo *res;
  struct sockaddr_in* sin;
  int err;
  int s;

  // look up host to get a struct sockaddr
  err = getaddrinfo(host, NULL, NULL, &res);
  if (err) {
    if (err == EAI_SYSTEM) {
      perror("open_server");
    } else {
      fprintf(stderr, "error in getaddrinfo: %s\n", gai_strerror(err));
    }
    die(WHY_SERVER);
  }
  if (!res) {
    fprintf(stderr, "could not locate %s\n", host);
    die(WHY_SERVER);
  }

  s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (s < 0) {
    perror("failed to create server socket");
    die(WHY_SERVER);
  }
  sin = (struct sockaddr_in*)(res->ai_addr);
  //sin->sin_family = AF_INET;
  sin->sin_port = htons(port);
  if (connect(s, res->ai_addr, res->ai_addrlen) < 0) {
    perror("server connection failed");
    die(WHY_SERVER);
  }

  return s;
}

int main(int argc, char** argv) {
  // set signal handler for INT & TERM
  signal(SIGINT, sig_handler);
  signal(SIGTERM, sig_handler);

  // parse command line args
  init_args(argc, argv);

  // open serial fd
  serial_fd = open_serial(ser_path, ser_speed);
  if (serial_fd < 1) {
    usage(argv[0]);
    die(WHY_SERIALPATH);
  }

  // open socket to server
  server_fd = open_server(server, port);
  if (server_fd < 1) {
    usage(argv[0]);
    die(WHY_SERVER);
  }

  // set up fd list for poll()
  struct pollfd fds[2];
  struct pollfd *server_pollfd, *serial_pollfd;
  fds[0].fd = serial_fd;
  fds[0].events = POLLIN;
  fds[0].revents = 0;
  fds[1].fd = server_fd;
  fds[1].events = POLLIN;
  fds[1].revents = 0;
  serial_pollfd = fds;
  server_pollfd = fds + 1;

  // main poll() loop
  const static char* XSBUG_BEGIN = "POST / HTTP/1.1";
  const int BEGIN_LEN = strlen(XSBUG_BEGIN);
  const static char* XSBUG_END = "</xsbug>";
  const int END_LEN = strlen(XSBUG_END);
  int connected = 0;
  uint8_t history[10*BUF_SIZE];
  int history_len = sizeof(history);
  uint8_t buf[BUF_SIZE];
  size_t tail = 0;

  for (;;) {
    int res;
    size_t rcount;

    res = poll(fds, 2, -1);
    if (res < 0) {
      perror("main loop");
      die(WHY_POLL_ERROR);
    }

    if (res) {
      if (server_pollfd->revents & (POLLERR | POLLHUP)) {
        fprintf(stderr, "lost connection to xsbug server\n");
        die(WHY_XSBUG_HOST);
      }

      if (serial_pollfd->revents & (POLLERR | POLLHUP)) {
        fprintf(stderr, "serial port disconnected\n");
        die(WHY_SERIAL_GONE);
      }

      if (serial_pollfd->revents & POLLIN) {
        rcount = read(serial_pollfd->fd, buf, BUF_SIZE);
        if (!rcount) {
          fprintf(stderr, "serial port disconnected\n");
          die(WHY_SERIAL_GONE);
        }
        if (rcount < 0) {
          perror("serial disconnected");
          die(WHY_SERIAL_GONE);
        }

        // copy to our working history buffer, which we will use to scan for start/end markers
        if (tail + rcount > history_len) {
          fprintf(stderr, "history buffer overflow\n");
          die(WHY_INTERNAL_ERROR);
        }
        memmove(history + tail, buf, rcount);
        memset(history + tail + rcount, 0, history_len - rcount - tail); // make sure strstr() doesn't overrun

        //  DBG("\n******************** serial buf %d %d\n", rcount, tail);
        //  DBG(buf);
        //  DBG("********************\n\n");

        int done = 0;
        while (!done) { // the new bytes could hypothetically contain multiple start->end sequences, so loop over it
          int start = -1, end = -1;
          uint8_t *s = NULL, *e = NULL;
          done = 1;

          s = (uint8_t*)strstr((char*)history, XSBUG_BEGIN);
          start = s ? s - history : -1;
          e = (uint8_t*)strstr((char*)history, XSBUG_END);
          end = e ? e - history : -1;

          // start is the offset of the first char of the first start tag, or -1 if there is no start tag
          // end is the offset of the first char of the first end tag, or is -1 if there is no end tag

          // the xsbug server doesn't tolerate chunked data very well, so we accumulate the data in
          // a buffer so we can blast it over as quickly as possible, to minimize fragmentation.
          // Accordingly, we copy the serial input into a rolling buffer, but as soon as we see a
          // start tag we snap that down to offset 0 and then accumulate bytes in the buffer until we
          // see an end tag; then we transmit it all at once.

          //DBG("\n******************** history %d %d %d %d\n", tail, start, end, rcount);
          //DBG(history);
          //DBG("********************\n\n");

          if (start < 0 && end < 0) {
            // easiest case - means new data contains neither a start nor an end tag, so just obey current state
            //DBG("straight data\n");
            if (connected) {
              tail += rcount;
            } else {
              if (tail + rcount > BEGIN_LEN) {
                memmove(history, history + tail + rcount - BEGIN_LEN, BEGIN_LEN);
                memset(history + BEGIN_LEN, 0, history_len - BEGIN_LEN);
                tail = BEGIN_LEN;
              } else {
                tail += rcount;
              }
            }
          } else if (start > -1 && end < 0) {
            //DBG(".....S.....\n");
            // means we have ".....START_TAG......" (or multiple start tags)
            if (connected) {
              // means we saw two start tags in a row w/ no end tag, which is fine
              tail += rcount;
            } else {
              tail = tail + rcount - start;
              memmove(history, history + start, tail);
              memset(history + tail, 0, history_len - tail);
              connected = 1;
            }
          } else if (start < 0 && end > -1) {
            DBG(".....E.....\n");
            // means we have "......END_TAG......"
            if (connected) {
              // if we are connected, we should have the corresponding START_TAG at history[0], so this should never happen
              fprintf(stderr, "in connected state with no start tag; buffer overflow?\n");
              die(WHY_INTERNAL_ERROR);
            } else {
              // if we have no corresponding start tag, entire block can be ignored since we are a noop until we see a start
              tail = 0;
              memset(history, 0, history_len);
            }
          } else if (end < start) { // note that (start > -1 && end > -1) is implicit 
            // means we have ".....END_TAG....START_TAG...."
            //DBG("...E...S...\n");
            if (connected) {
              // if we are connected, we should have the corresponding START_TAG at history[0], so this should never happen
              fprintf(stderr, "in connected state with no start tag");
              die(WHY_INTERNAL_ERROR);
            } else {
              tail = tail + rcount - start;
              memmove(history, history + start, tail);
              memset(history + start, 0, tail);

              done = 0; // but there may be another end tag hiding after the start, so trigger the loop
              connected = 1;
              rcount = 0;
            }
          } else if (start < end) { // note that (start > -1 && end > -1) is implicit 
            //DBG("...S...E...   %d %d %d\n", start, end, end + END_LEN - start);
            // means we have ".....START_TAG.....END_TAG....." (or more than one start before end)
            copy_to(server_pollfd->fd, history + start, (end + END_LEN - start));

            // set our pointer after end tag and iterate, since there could be another start tag after that
            tail = tail + rcount - end - END_LEN;
            memmove(history, history + end + END_LEN, tail);
            memset(history + tail, 0, history_len - tail);

            done = 0;
            connected = 0;
            rcount = 0;
          } else {
            // means start == end >= 0 which is.... wtf
            fprintf(stderr, "start and end tags found to be coincident\n");
            die(WHY_INTERNAL_ERROR);
          }
        }
      }

      // copy socket -> serial, if data is waiting
      if (server_pollfd->revents & POLLIN) {
        //DBG("server data\n");
        rcount = read(server_pollfd->fd, buf, BUF_SIZE);
        if (!rcount) {
          fprintf(stderr, "server disconnected\n");
          die(WHY_SERVER);
        }
        if (rcount < 0) {
          perror("server disconnected");
          die(WHY_SERIAL_GONE);
        }

        copy_to(serial_pollfd->fd, buf, rcount);
      }
    }
  }
}
