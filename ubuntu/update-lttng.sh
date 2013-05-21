#!/bin/bash

#
# Update lttng from the modules repository
#

REPO=git://git.lttng.org/lttng-modules.git

git clone $REPO
rsync -av --delete --exclude=.git lttng-modules/ lttng/

