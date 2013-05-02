#!/bin/bash

#
# Update lttng from the modules repository
#

REPO=git://git.lttng.org/lttng-modules.git
TMP=$$.tmp

mkdir -p $TMP
cd $TMP
git clone $REPO
rsync -av --exclude=.git * ..
cd ..
git add -u

