#! /bin/sh
BCC_PATH=/usr/lib64/bcc
bcc -c -o bcc_example.o bcc_example.c
../../link -Felks $BCC_PATH/crt0.o bcc_example.o $BCC_PATH/libc.a -o bcc_example
chmod +x bcc_example
