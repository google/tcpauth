/*
 * Proxy on the client side to connect to servers using MD5SIG.
 *
 * Example:
 *   echo "correct horse battery staple" > pw.txt
 *   chmod 600 pw.txt
 *   ssh -oProxyCommand="./tcpauth-client-proxy -P pw.txt %h %p" shell.example.com
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
#include<pthread.h>
#include<unistd.h>

#include<sys/socket.h>
#include<sys/types.h>

#include"common.h"

const char* password = NULL;

void
usage(int err)
{
        printf("Usage: %s [options] <host> <port>\n"
               "    -h                   Show this usage text.\n"
               "    -P <password file>   File containing MD5SIG password.\n"
               "", argv0);
        exit(err);
}

typedef struct {
        int src, dst;
} reader;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static int shutting_down_var = 0;
static void
set_shutting_down()
{
        int n;
        if ((n = pthread_mutex_lock(&mutex))) {
                xerror("mutex lock error: code %d", n);
        }
        shutting_down_var = 1;
        if ((n = pthread_mutex_unlock(&mutex))) {
                xerror("mutex lock error: code %d", n);
        }
}

static int
shutting_down()
{
        int n;
        int ret;
        if ((n = pthread_mutex_lock(&mutex))) {
                xerror("mutex lock error: code %d", n);
        }
        ret = shutting_down_var;
        if ((n = pthread_mutex_unlock(&mutex))) {
                xerror("mutex lock error: code %d", n);
        }
        return ret;
}

// read data from one and and write to the other.
static void*
reader_main(void* rin)
{
        const reader* r = (reader*)rin;
        char buf[1024];
        for (;;) {
                ssize_t n = read(r->src, buf, sizeof(buf));
                if (shutting_down()) {
                        return NULL;
                }
                if (n == 0) {
                        set_shutting_down();
                        return NULL;
                }
                if (0 > n) {
                        xerror("read(%d): %s", r->src, strerror(errno));
                }
                const char* p = buf;
                while (n > 0) {
                        const ssize_t wn = write(r->dst, p, n);
                        if (0 > wn) {
                                xerror("write(): %s", strerror(errno));
                        }
                        n -= wn;
                        p += wn;
                }
        }
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

        // Start one half of the data shuffling.
        pthread_t other;
        reader other_r;
        other_r.src = fd;
        other_r.dst = STDOUT_FILENO;
        if (pthread_create(&other, NULL, &reader_main, &other_r)) {
                xerror("pthread_create(): %s", strerror(errno));
        }

        // Start the other half.
        reader this_r;
        this_r.src = STDIN_FILENO;
        this_r.dst = fd;
        reader_main(&this_r);
        int n;
        if ((n = pthread_join(other, NULL))) {
                xerror("pthread_join: %s", strerror(n));
        }
        exit(0);
}

int
main(int argc, char** argv)
{
        const char* password_file = NULL;  // -P <password file>
        argv0 = argv[0];

        int c;
        while (EOF != (c = getopt(argc, argv, "hP:"))) {
                switch (c) {
                case 'h':
                        usage(0);
                case 'P':
                        password_file = optarg;
                        break;
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
        password = get_password(password_file);

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

                for (const struct addrinfo *rp = ai; rp != NULL; rp = rp->ai_next) {
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
