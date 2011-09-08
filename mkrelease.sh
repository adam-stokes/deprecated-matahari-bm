#!/bin/bash

set -e

if [ $# -ne 2 ] ; then
    echo "Usage: $0 <branch> <tag>"
    echo "Example: $0 v0.4 v0.4.3"
    exit 1
fi

BRANCH=$1
TAG=$2

cat << EOF

For now, the version string is hard coded in a number of places.
Be sure to change them all.
   - GNUmakefile
   - autobuild.sh
   - matahari.spec.in
   - mingw32-matahari.spec.in
   - CMakeLists.txt
If you have already done this, hit enter to continue.  If you haven't,
hit control-c and go do it.

EOF
read

git tag -a -m "Tagged release $TAG" $TAG $BRANCH
git archive --prefix=matahari-${TAG:1}/ $TAG | tar x
tar czvf matahari-${TAG:1}.tar.gz matahari-${TAG:1}/
