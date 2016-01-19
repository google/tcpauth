/*
 * Copyright 2015 Google Inc. All Rights Reserved.
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
 */
#include<errno.h>
#include<netdb.h>
#include<netinet/tcp.h>
#include<stdarg.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#include<sys/socket.h>
#include<sys/types.h>

const char* argv0 = NULL;
const char* password = "secret";

void
usage(int err)
{
        printf("Usage!\n");
        exit(err);
}

void
error(const char* fmt, ...)
{
        char buffer[256];
        va_list args;
        va_start(args, fmt);
        vsprintf(buffer, fmt, args);
        fprintf(stderr, "%s\n", buffer);
        va_end(args);
        exit(1);
}

int
handle(int fd)
{
        if (1) {
                struct tcp_md5sig md5sig;
                socklen_t t = sizeof(struct sockaddr_storage);

                memset(&md5sig, 0, sizeof(md5sig));
                strncpy(md5sig.tcpm_key, password, TCP_MD5SIG_MAXKEYLEN);

                if (getpeername(fd,
                                (struct sockaddr*)&md5sig.tcpm_addr, &t)) {
                        error("getpeername(): %.100s", strerror(errno));
                }
                md5sig.tcpm_keylen = strlen(md5sig.tcpm_key);
                if (-1 == setsockopt(fd,
                                     IPPROTO_TCP, TCP_MD5SIG,
                                     &md5sig, sizeof(md5sig))) {
                        error("setsockopt(TCP_MD5SIG): %.100s", strerror(errno));
                }
        }
        int src_fd;
        int dst_fd;
        pid_t pid;
        switch (pid = fork()) {
        case -1:
                error("fork(): %s", strerror(errno));
        case 0:
                src_fd = fd;
                dst_fd = STDOUT_FILENO;
                break;
        default:
                src_fd = STDIN_FILENO;
                dst_fd = fd;
                break;
        }
        for (;;) {
                char buf[1024];
                ssize_t n = read(src_fd, buf, sizeof(buf));
                if (0 > n) {
                        error("read(%d): %s", src_fd, strerror(errno));
                }
                if (n == 0) {
                        if (0 > close(src_fd)) {
                                error("close(): %s", strerror(errno));
                        }
                        if (0 > close(dst_fd)) {
                                error("close(): %s", strerror(errno));
                        }
                }
                const char* p = buf;
                while (n > 0) {
                        ssize_t wn = write(dst_fd, p, n);
                        if (0 > wn) {
                                error("write(): %s", strerror(errno));
                        }
                        n -= wn;
                        p += wn;
                }
        }
}

int
main(int argc, char** argv)
{

        argv0 = argv[0];

        int c;
        while (EOF != (c = getopt(argc, argv, "p:H:"))) {
                switch (c) {
                default:
                        usage(1);
                }
        }

        const char* node = argv[optind];
        const char* port = argv[optind+1];


        // Create socket and bind.
        int fd = -1;
        {
                struct addrinfo hints;
                memset(&hints, 0, sizeof(hints));
                hints.ai_flags = AI_ADDRCONFIG;
                hints.ai_family = AF_UNSPEC;
                hints.ai_socktype = SOCK_STREAM;
                struct addrinfo *ai;

                if (0 != getaddrinfo(node, port, &hints, &ai)) {
                        fprintf(stderr, "%s: getaddrinfo(%s, %s): %s\n", argv0, node, port, strerror(errno));
                        exit(1);
                }

                struct addrinfo *rp;
                for (rp = ai; rp != NULL; rp = rp->ai_next) {
                        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
                        if (0 > fd) {
                                continue;
                        }
                        if (0) {
                                int on = 1;
                                if (0 > setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) {
                                        error("setsockopt(SO_REUSEADDR) failed");
                                }
                                if (!bind(fd, rp->ai_addr, ai->ai_addrlen)) {
                                        break;
                                }
                                if (0 != close(fd)) {
                                        fprintf(stderr, "%s: closing failed socket: %s\n", argv0, strerror(errno));
                                }
                        }
                        if (!connect(fd, rp->ai_addr, ai->ai_addrlen)) {
                                break;
                        }
                        fprintf(stderr, "%s: connect failed: %s", argv0, strerror(errno));
                }
                if (0 > fd) {
                        error("%s: Could not bind to %s %s\n", argv0, node, port);
                }
                freeaddrinfo(ai);
        }

        fprintf(stderr, "Connected to %s port %s\n", node, port);

        return handle(fd);
}
/* ---- Emacs Variables ----
 * Local Variables:
 * c-basic-offset: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vim: ts=8 sw=8
 */
