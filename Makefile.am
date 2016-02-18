AUTOMAKE_OPTIONS = foreign subdir-objects
DISTCLEANFILES = *~

bin_PROGRAMS=tcpauth-client-proxy tcpauth-wrap

# TODO: Rename to make it clear this is for the client.
tcpauth_client_proxy_SOURCES=\
src/client-proxy.c\
src/common.c

# TODO: Rename to make it clear this is the server.
tcpauth_wrap_SOURCES=\
src/wrap.c\
src/common.c
