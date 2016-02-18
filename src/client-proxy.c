/*
 * Proxy on the client side to connect to servers using MD5SIG.
 *
 * Example:
 *   ssh -oProxyCommand="./client-proxy %h %p" shell.example.com
 */
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
#include<netdb.h>
#include<netinet/tcp.h>
#include<stdarg.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#include<sys/socket.h>
#include<sys/types.h>

#include"common.h"

const char* password = "secret";

void
usage(int err)
{
        printf("Usage: %s [options] <host> <port>\n"
               "    -h          Show this usage text.\n"
               "", argv0);
        exit(err);
}

// Handle a new connection after connect() returns.
// * Enable MD5SIG
// * Funnel data back and forth.
int
handle(int fd)
{
        if (1) {
                struct tcp_md5sig md5sig;
                socklen_t t = sizeof(struct sockaddr_storage);

                memset(&md5sig, 0, sizeof(md5sig));
                strncpy((char*)md5sig.tcpm_key, password, TCP_MD5SIG_MAXKEYLEN);

                if (getpeername(fd,
                                (struct sockaddr*)&md5sig.tcpm_addr, &t)) {
                        xerror("getpeername(): %.100s", strerror(errno));
                }
                md5sig.tcpm_keylen = strlen((char*)md5sig.tcpm_key);
                if (-1 == setsockopt(fd,
                                     IPPROTO_TCP, TCP_MD5SIG,
                                     &md5sig, sizeof(md5sig))) {
                        xerror("setsockopt(TCP_MD5SIG): %.100s", strerror(errno));
                }
        }
        int src_fd;
        int dst_fd;
        pid_t pid;
        switch (pid = fork()) {
        case -1:
                xerror("fork(): %s", strerror(errno));
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
                        xerror("read(%d): %s", src_fd, strerror(errno));
                }
                if (n == 0) {
                        if (0 > close(src_fd)) {
                                xerror("close(): %s", strerror(errno));
                        }
                        if (0 > close(dst_fd)) {
                                xerror("close(): %s", strerror(errno));
                        }
                }
                const char* p = buf;
                while (n > 0) {
                        ssize_t wn = write(dst_fd, p, n);
                        if (0 > wn) {
                                xerror("write(): %s", strerror(errno));
                        }
                        n -= wn;
                        p += wn;
                }
        }
        // TODO: when one process dies, kill the other.
}

int
main(int argc, char** argv)
{
        argv0 = argv[0];

        int c;
        while (EOF != (c = getopt(argc, argv, "h"))) {
                switch (c) {
                case 'h':
                        usage(0);
                default:
                        usage(1);
                }
        }

        if (optind + 2 != argc) {
                fprintf(stderr, "%s: Provide exactly two non-option args: host and port.\n", argv0);
                exit(1);
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
                        xerror("getaddrinfo(%s, %s): %s", node, port, strerror(errno));
                }

                for (struct addrinfo *rp = ai; rp != NULL; rp = rp->ai_next) {
                        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
                        if (0 > fd) {
                                fprintf(stderr, "%s: socket(): %s\n", argv0, strerror(errno));
                                continue;
                        }
                        if (!connect(fd, rp->ai_addr, ai->ai_addrlen)) {
                                break;
                        }
                        fprintf(stderr, "%s: connect failed: %s\n", argv0, strerror(errno));
                        if (0 != close(fd)) {
                                fprintf(stderr, "%s: closing failed socket: %s\n", argv0, strerror(errno));
                        }
                        fd = -1;
                }
                if (0 > fd) {
                        xerror("%s: Could not connect to %s %s\n", argv0, node, port);
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
