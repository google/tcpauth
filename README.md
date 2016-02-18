# tcpauth

Copyright 2016 Google Inc. All Rights Reserved.

https://github.com/google/tcpauth

## Introduction

tcpauth allows you to wrap TCP connections in RFC2386 MD5 signatures, to prevent
any attacker from talking to a server without first having the shared secret.

This protects against any preauth attacks in the server application itself. You
could compare it to port knocking, in that this could let you keep SSH open for
connections from all over the world, as long as they know the shared
secret. Normal authentication would take place after connection, so it doesn't
reduce security.

Another benefit is that when MD5 signatures are turned on an attacker can't
spoof RST packets to kill your connection.

## Installing

If building from git repo:

```shell
./boostrap.sh
```

then

```shell
./configure && make && make install
```

## Running

Example of running an SSH server on port 12345.

On the server:

```shell
sudo ./wrap -p 12345 -- /usr/sbin/sshd -i
```

On the client:

```shell
ssh "-oProxyCommand=$(pwd)/proxy %h %p" -p 12345 shell.example.com
```
