#!/bin/sh

mkdir dkms-base

cp -rP arch block certs crypto drivers firmware fs include init ipc kernel lib mm modules net samples scripts security sound tools usr virt Makefile Kconfig Kbuild build-out dkms-base

tar -c dkms-base | gzip -9 > dkms-base.tar.gz
