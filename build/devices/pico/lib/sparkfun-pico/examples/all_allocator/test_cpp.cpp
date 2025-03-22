/**
 * @file test_cpp.cpp
 *
 * @brief This file contains a example to show new/delete in C++
 *
 */

/*
The MIT License (MIT)

Copyright (c) 2024 SparkFun Electronics

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions: The
above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED
"AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/
#include "test_cpp.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

class TestCPP
{
  public:
    TestCPP()
    {
        printf("\n\tTestCPP::TestCPP() - constructor\n");
    }
    ~TestCPP()
    {
        printf("\n\tTestCPP::~TestCPP() - destructor\n");
    }
    uint8_t _meg_buffer[1024 * 1024];
};

void *test_cpp_new(void)
{
    TestCPP *tmp = new TestCPP();
    return (void *)tmp;
}

void test_cpp_delete(void *ptr)
{
    delete (TestCPP *)ptr;
}