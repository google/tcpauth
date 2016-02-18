/*
 * Wrap a server in MD5SIG. The TCP socket connected to the client is
 * passed on to the binary in fd 0 and 1.
 *
 * Example:
 *   ./wrap -p 12345 -- /usr/sbin/sshd -i
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
const char* next_binary = NULL;
char* const* next_args = NULL;
const char* default_opt_H = "::";

void
usage(int err)
{
        printf("Usage: %s [options] -- /path/to/binary [options to binary...]\n"
               "    -h          Show this usage text.\n"
               "    -H <addr>   Address to listen to (default: \"%s\").\n"
               "    -p <port>   Port to listen on.\n"
               "", argv0, default_opt_H);
        exit(err);
}

void
error(const char* fmt, ...)
{
        char buffer[256];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        fprintf(stderr, "%s: %s\n", argv0, buffer);
        va_end(args);
        exit(1);
}

// Handle a new connection that's come in:
// * Enable MD5SIG
// * Exec next binary.
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
        if (0 > dup2(fd, 0)
            || 0 > dup2(fd, 1)) {
                error("dup2(): %s", strerror(errno));
        }
        close(fd);
        execvp(next_binary, next_args);
        error("execv(): %s", strerror(errno));
}

// Main loop listening for connections and calling handle() on them.
int
main_loop(int fd)
{
        for (;;) {
                int newsock;
                struct sockaddr_storage from;
                socklen_t fromlen;

                fromlen = sizeof(from);
                newsock = accept(fd, (struct sockaddr*)&from, &fromlen);
                if (0 > newsock) {
                        fprintf(stderr, "%s: accept(): %s\n", argv0, strerror(errno));
                        continue;
                }

                pid_t pid;
                switch (pid = fork()) {
                case -1:
                        fprintf(stderr, "%s: fork(): %s\n", argv0, strerror(errno));
                        close(newsock);
                        continue;
                case 0:
                        handle(newsock);
                        exit(0);
                default:
                        close(newsock);
                        break;
                }
        }
}

int
main(int argc, char** argv)
{
        const char* port = NULL;  // -p <port>
        const char* node = default_opt_H;  // -H <addr>

        argv0 = argv[0];

        int c;
        while (EOF != (c = getopt(argc, argv, "hH:p:"))) {
                switch (c) {
                case 'h':
                        usage(0);
                case 'H':
                        node = optarg;
                        break;
                case 'p':
                        port = optarg;
                        break;
                default:
                        usage(1);
                }
        }

        if (port == NULL) {
                fprintf(stderr, "%s: Need to specify port (-p)\n", argv0);
                exit(1);
        }

        // Create socket and bind.
        int fd = -1;
        {
                struct addrinfo hints;
                memset(&hints, 0, sizeof(hints));
                hints.ai_flags = AI_ADDRCONFIG | AI_PASSIVE;
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
                        int on = 1;
                        if (0 > setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) {
                                error("setsockopt(SO_REUSEADDR)");
                        }
                        if (!bind(fd, rp->ai_addr, ai->ai_addrlen)) {
                                break;
                        }
                        if (0 != close(fd)) {
                                fprintf(stderr, "%s: closing failed socket: %s\n", argv0, strerror(errno));
                        }
                        fd = -1;
                }
                if (0 > fd) {
                        error("Could not bind to \"%s\" port \"%s\"", node, port);
                }
                freeaddrinfo(ai);

                if (listen(fd, 5)) {
                        error("listen(): %s", strerror(errno));
                }
        }

        { // TODO
                next_binary = argv[optind];
                next_args = &argv[optind];
        }

        {
                fprintf(stderr, "Listening on %s port %s\n", node, port);
                fprintf(stderr, "Next binary: %s\n", next_binary);
                fprintf(stderr, "Args:\n");
                int c;
                for (c = 0; next_args[c]; c++) {
                        fprintf(stderr, "  %s\n", next_args[c]);
                }
        }
        return main_loop(fd);
}
/* ---- Emacs Variables ----
 * Local Variables:
 * c-basic-offset: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vim: ts=8 sw=8
 */
