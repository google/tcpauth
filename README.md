# tcpauth

Copyright 2015 Google Inc. All Rights Reserved.

https://github.com/google/tcpauth

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
