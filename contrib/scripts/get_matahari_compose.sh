#!/bin/bash

function usage() {
	echo "$0 <task> <buildid> <pkg_nvr>"
}

if [ $# -lt 3 ]; then
	usage
	exit 1
fi

TASK=$1
BUILDID=$2
PKG_NVR=$3
ARCH=x86_64

BASE_URL="http://download.devel.redhat.com/brewroot/work/tasks"

RPM_LIST=('matahari' \
	  'matahari-agent-lib' \
          'matahari-broker' \
	  'matahari-consoles' \
	  'matahari-devel' \
	  'matahari-debuginfo' \
	  'matahari-host' \
	  'matahari-lib' \
	  'matahari-network' \
	  'matahari-service' \
	  'matahari-sysconfig')

for i in "${RPM_LIST[@]}"; do
	wget "$BASE_URL/$TASK/$BUILDID/$i-$PKG_NVR.el6.$ARCH.rpm"
done
