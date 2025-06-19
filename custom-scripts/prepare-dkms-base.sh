#!/bin/sh

mkdir dkms-base

cp -rP arch block certs crypto drivers firmware fs include init ipc kernel lib mm modules net samples scripts security sound tools usr virt Makefile Kconfig Kbuild dkms-base
