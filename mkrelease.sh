#!/bin/bash

set -e

if [ $# -ne 2 ] ; then
    echo "Usage: $0 <branch> <tag>"
    echo "Example: $0 v0.4 v0.4.3"
    exit 1
fi

BRANCH=$1
TAG=$2

git checkout ${BRANCH}
if [ "$(cat .version)" != "${TAG:1}" ] ; then
    echo "${TAG:1}" > .version
    git add .version
    git commit -s -m "build: update .version to ${TAG:1}"
fi
git tag -a -m "Tagged release $TAG" $TAG $BRANCH
git archive --prefix=matahari-${TAG:1}/ $TAG | tar x
tar czvf matahari-${TAG:1}.tar.gz matahari-${TAG:1}/
