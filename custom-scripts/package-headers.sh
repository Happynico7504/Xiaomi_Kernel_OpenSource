#!/bin/sh
# Package what's needed to build out-of-tree modules against this kernel,
# like the linux-headers package from scripts/package/builddeb, as
# headers.tar.gz with a usr/src/linux-headers-<release> tree.
#
# usage: package-headers.sh <objtree>    (run from the source tree)
#
# Host programs under scripts/ are built for the CI machine, so they are
# left out and have to be rebuilt on the device with "make scripts".

set -e

objtree=$1
SRCARCH=arm64
version=$(cat "$objtree/include/config/kernel.release")
destdir=hdrinst/usr/src/linux-headers-$version

srcdirs="arch block certs crypto drivers firmware fs include init ipc kernel
	lib mm net samples scripts security sound tools usr virt"

# Drop ELF files, they are all host programs for the build machine.
no_elf()
{
	while read -r f; do
		if [ -f "$f" ] && [ ! -L "$f" ] &&
		   [ "$(head -c 4 "$f" | od -An -tx1 | tr -d ' ')" = 7f454c46 ]; then
			continue
		fi
		echo "$f"
	done
}

rm -rf hdrinst
mkdir -p "$destdir"

{
	find $srcdirs -name 'Makefile*' -o -name 'Kconfig*' -o -name Kbuild \
		-o -name '*.pl'
	echo Makefile
	echo Kconfig
	find arch/*/include include scripts -type f -o -type l
	find arch/$SRCARCH -name module.lds -o -name Kbuild.platforms \
		-o -name Platform
	find $(find arch/$SRCARCH -name include -o -name scripts -type d) -type f
	# MTK headers are on every module's include path (LINUXINCLUDE).
	find drivers/misc/mediatek/include -type f -o -type l
	# Headers for scripts/selinux/genheaders and scripts/sortextable.
	find security/selinux/include tools/include -type f
} | sort -u | no_elf | tar -c -T - | tar -x -C "$destdir"

# Generated headers, config and Module.symvers, without objects and .cmd
# files.
(
	cd "$objtree"
	find arch/$SRCARCH/include include scripts Module.symvers .config \
		-type f ! -name '*.o' ! -name '.*.cmd' | no_elf | tar -c -T -
) | tar -x -C "$destdir"

tar -C hdrinst -c usr | gzip -9 > headers.tar.gz
