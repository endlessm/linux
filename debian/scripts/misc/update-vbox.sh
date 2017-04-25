#!/bin/bash

set -e

if ! grep PATCHLEVEL Makefile; then
	echo "You must run this script from the top of the linux tree"
	exit 1
fi

if [ "$#" -ne 1 ]; then
	echo "Usage: $0 <package-version>"
	exit 1
fi

TMPDIR=$(mktemp -d)
VERSION="$1"
PKG_NAME="virtualbox-guest-dkms_${VERSION}_all.deb"
PKG_URL="http://ports.ubuntu.com/pool/multiverse/v/virtualbox/${PKG_NAME}"

cleanup() {
	rm -r "$TMPDIR"
}
trap cleanup EXIT

cd "$TMPDIR"
curl -s -O "$PKG_URL"
dpkg -x "$PKG_NAME" extract
SRC=$(readlink -f extract/usr/src/*)
cd -

DST="ubuntu/vbox"
rm -rf "$DST"
rsync -av "$SRC/" "$DST"
cat >"${DST}/BOM" <<EOF
Source: ${PKG_URL}
Version: ${VERSION}
EOF

for f in $(find ubuntu/vbox/* -mindepth 1 -name Makefile); do
	sed -i '1i KBUILD_EXTMOD=${srctree}/ubuntu/vbox' "$f"
done

git add "$DST"
git commit -s -m "UBUNTU: ubuntu: vbox -- Update to ${VERSION}"
