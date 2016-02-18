/*
 * Copyright 2016 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This is not an official Google product.
 */
#include"config.h"

#include<stdarg.h>
#include<stdio.h>
#include<stdlib.h>

#include"common.h"

const char* argv0 = NULL;

void
xerror(const char* fmt, ...)
{
        char buffer[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        fprintf(stderr, "%s: %s\n", argv0, buffer);
        va_end(args);
        exit(1);
}
