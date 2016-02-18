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

#include<errno.h>
#include<stdarg.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include"common.h"

const char* argv0 = NULL;
const char* password_env = "MD5SIG_PASSWORD";

char*
get_password(const char* opt)
{
        // If there's a password file, use that.
        if (opt) {
                FILE* f;

                if (!(f = fopen(opt, "r"))) {
                        xerror("fopen(%s): %s", opt, strerror(errno));
                }
                char buf[128] = {0};
                const char* ret = fgets(buf, sizeof(buf), f);
                const int err = errno;
                fclose(f);
                if (!ret) {
                        xerror("fgets(%s): %s", opt, strerror(err));
                }
                for (size_t c = strlen(buf)-1; c; c--) {
                        if (buf[c] == '\n') {
                                buf[c] = 0;
                        }
                }
                return strdup(ret);
        }
        const char* ret = getenv(password_env);
        if (!ret) {
                xerror("No password provided in file or environment");
        }
        return strdup(ret);
}

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
/* ---- Emacs Variables ----
 * Local Variables:
 * c-basic-offset: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vim: ts=8 sw=8
 */
