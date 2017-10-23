/*
 * Copyright (c) 2006-2016 Kirill Simonov
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "yaml_private.h"

/*
 * Declarations.
 */

static int
yaml_emitter_set_writer_error(yaml_emitter_t *emitter, const char *problem);

YAML_DECLARE(int)
yaml_emitter_flush(yaml_emitter_t *emitter);

/*
 * Set the writer error and return 0.
 */

static int
yaml_emitter_set_writer_error(yaml_emitter_t *emitter, const char *problem)
{
    emitter->error = YAML_WRITER_ERROR;
    emitter->problem = problem;

    return 0;
}

/*
 * Flush the output buffer.
 */

YAML_DECLARE(int)
yaml_emitter_flush(yaml_emitter_t *emitter)
{
    int low, high;

    assert(emitter);    /* Non-NULL emitter object is expected. */
    assert(emitter->write_handler); /* Write handler must be set. */
    assert(emitter->encoding);  /* Output encoding must be set. */

    emitter->buffer.last = emitter->buffer.pointer;
    emitter->buffer.pointer = emitter->buffer.start;

    /* Check if the buffer is empty. */

    if (emitter->buffer.start == emitter->buffer.last) {
        return 1;
    }

    /* If the output encoding is UTF-8, we don't need to recode the buffer. */

    if (emitter->encoding == YAML_UTF8_ENCODING)
    {
        if (emitter->write_handler(emitter->write_handler_data,
                    emitter->buffer.start,
                    emitter->buffer.last - emitter->buffer.start)) {
            emitter->buffer.last = emitter->buffer.start;
            emitter->buffer.pointer = emitter->buffer.start;
            return 1;
        }
        else {
            return yaml_emitter_set_writer_error(emitter, "write error");
        }
    }

    /* Recode the buffer into the raw buffer. */

    low = (emitter->encoding == YAML_UTF16LE_ENCODING ? 0 : 1);
    high = (emitter->encoding == YAML_UTF16LE_ENCODING ? 1 : 0);

    while (emitter->buffer.pointer != emitter->buffer.last)
    {
        unsigned char octet;
        unsigned int width;
        unsigned int value;
        size_t k;

        /* 
         * See the "reader.c" code for more details on UTF-8 encoding.  Note
         * that we assume that the buffer contains a valid UTF-8 sequence.
         */

        /* Read the next UTF-8 character. */

        octet = emitter->buffer.pointer[0];

        width = (octet & 0x80) == 0x00 ? 1 :
                (octet & 0xE0) == 0xC0 ? 2 :
                (octet & 0xF0) == 0xE0 ? 3 :
                (octet & 0xF8) == 0xF0 ? 4 : 0;

        value = (octet & 0x80) == 0x00 ? octet & 0x7F :
                (octet & 0xE0) == 0xC0 ? octet & 0x1F :
                (octet & 0xF0) == 0xE0 ? octet & 0x0F :
                (octet & 0xF8) == 0xF0 ? octet & 0x07 : 0;

        for (k = 1; k < width; k ++) {
            octet = emitter->buffer.pointer[k];
            value = (value << 6) + (octet & 0x3F);
        }

        emitter->buffer.pointer += width;

        /* Write the character. */

        if (value < 0x10000)
        {
            emitter->raw_buffer.last[high] = value >> 8;
            emitter->raw_buffer.last[low] = value & 0xFF;

            emitter->raw_buffer.last += 2;
        }
        else
        {
            /* Write the character using a surrogate pair (check "reader.c"). */

            value -= 0x10000;
            emitter->raw_buffer.last[high] = 0xD8 + (value >> 18);
            emitter->raw_buffer.last[low] = (value >> 10) & 0xFF;
            emitter->raw_buffer.last[high+2] = 0xDC + ((value >> 8) & 0xFF);
            emitter->raw_buffer.last[low+2] = value & 0xFF;

            emitter->raw_buffer.last += 4;
        }
    }

    /* Write the raw buffer. */

    if (emitter->write_handler(emitter->write_handler_data,
                emitter->raw_buffer.start,
                emitter->raw_buffer.last - emitter->raw_buffer.start)) {
        emitter->buffer.last = emitter->buffer.start;
        emitter->buffer.pointer = emitter->buffer.start;
        emitter->raw_buffer.last = emitter->raw_buffer.start;
        emitter->raw_buffer.pointer = emitter->raw_buffer.start;
        return 1;
    }
    else {
        return yaml_emitter_set_writer_error(emitter, "write error");
    }
}

