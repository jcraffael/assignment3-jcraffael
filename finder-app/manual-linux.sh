#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.15.195
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
echo "Using default directory ${OUTDIR} for output"
else
OUTDIR=$1
echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # TODO: Add your kernel build steps here
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs
fi

echo "Adding the Image in outdir"
echo "12345678" | sudo -S cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    echo "12345678" | sudo -S rm -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
mkdir ${OUTDIR}/rootfs && cd ${OUTDIR}/rootfs
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir -p usr/bin usr/sbin user/lib
mkdir -p var/log

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
    git clone git@github.com:mirror/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
    make distclean
    make defconfig
else
    cd busybox
fi

# TODO: Make and install busybox
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

echo "Library dependencies"
PROG_OUTPUT=$(${CROSS_COMPILE}readelf -a busybox | grep "program interpreter")
SHARDLIB_OUTPUT=$(${CROSS_COMPILE}readelf -a busybox | grep "Shared library")
PROG=$(echo $PROG_OUTPUT | cut -d ' ' -f4 | cut -d ']' -f1)
SHARDLIST=$(echo $SHARDLIB_OUTPUT | sed 's/]/\n/g' | cut -d '[' -f2)

# TODO: Add library dependencies to rootfs
echo "cp prog interpreter which is $PROG to lib"
echo "12345678" | sudo -S find / \
  -path /proc -prune -o \
  -path /sys -prune -o \
  -path /dev -prune -o \
  -path /run -prune -o \
  -path "${OUTDIR}/rootfs/lib" -prune -o -type f -wholename *${PROG} -exec cp {} ${OUTDIR}/rootfs/lib/ \;
echo "cp shared libs to lib64"
for SHARDLIB in $SHARDLIST; do cp $(sudo find / -type f -name *${SHARDLIB} 2>/dev/null | grep ${CROSS_COMPILE::-1}) ${OUTDIR}/rootfs/lib64; done

# TODO: Make device nodes
cd ${OUTDIR}/rootfs
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 666 dev/console c 5 1

# TODO: Clean and build the writer utility
cd ${FINDER_APP_DIR}
make clean
make CROSS_COMPILE=${CROSS_COMPILE}

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
cp writer finder.sh finder-test.sh conf/* autorun-qemu.sh ${OUTDIR}/rootfs/home



# TODO: Chown the root directory
cd ${OUTDIR}/rootfs
echo "12345678" | sudo -S chown -R root:root *
# TODO: Create initramfs.cpio.gz
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
cd ${OUTDIR}
gzip -fk ${OUTDIR}/initramfs.cpio
