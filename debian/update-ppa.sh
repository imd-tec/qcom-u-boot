#!/bin/bash
# Update and upload u-boot-concept-qemu to PPA
#
# Usage: debian/update-ppa.sh [-n]
#   -n  dry run: build source package but don't sign or upload

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SRC_DIR="$(dirname "$SCRIPT_DIR")"
PPA="ppa:sjg1/u-boot-concept"
GPG_KEY="9AEF88906888C1946332D80A0B6C19937329F977"
MAINTAINER="Simon Glass <sjg@chromium.org>"
ALL_SERIES="noble jammy questing resolute"

cd "$SRC_DIR"

DRY_RUN=0
if [ "$1" = "-n" ]; then
    DRY_RUN=1
    echo "Dry run mode: will build but not sign or upload"
fi

# Pull latest from git
echo "Pulling latest changes..."
git pull

# Get version info from the tree
UBOOT_VERSION=$(sed -n 's/^VERSION = //p' Makefile).$(sed -n 's/^PATCHLEVEL = //p' Makefile)
GIT_DATE=$(date -u +%Y%m%d)
GIT_SHORT=$(git rev-parse --short HEAD)

# Determine next PPA revision
CURRENT_PPA=$(dpkg-parsechangelog -S Version 2>/dev/null || echo "")
if echo "$CURRENT_PPA" | grep -q "~ppa"; then
    CURRENT_NUM=$(echo "$CURRENT_PPA" | sed 's/.*~ppa\([0-9]*\).*/\1/')
    NEXT_NUM=$((CURRENT_NUM + 1))
else
    NEXT_NUM=1
fi

NEW_VERSION="${UBOOT_VERSION}-1~ppa${NEXT_NUM}"
echo "Version: $NEW_VERSION (git $GIT_SHORT, $GIT_DATE)"

# Save original changelog
cp debian/changelog debian/changelog.orig

for SERIES in $ALL_SERIES; do
    echo ""
    echo "=== Building for $SERIES ==="

    # Restore original changelog and add new entry for this series
    cp debian/changelog.orig debian/changelog
    cat > debian/changelog.new <<EOF
u-boot-concept (${NEW_VERSION}~${SERIES}1) ${SERIES}; urgency=medium

  * Update to latest master (git ${GIT_SHORT}, ${GIT_DATE}).

 -- ${MAINTAINER}  $(date -R)

EOF
    cat debian/changelog >> debian/changelog.new
    mv debian/changelog.new debian/changelog

    # Clean and build source package
    echo "Building source package for $SERIES..."
    PATH=/usr/bin:/usr/sbin:/bin:/sbin dpkg-buildpackage -S -d -us -uc

    CHANGES="../u-boot-concept_${NEW_VERSION}~${SERIES}1_source.changes"

    if [ "$DRY_RUN" = "0" ]; then
        echo "Signing..."
        debsign -k "$GPG_KEY" "$CHANGES"

        echo "Uploading to ${PPA}..."
        dput --force "$PPA" "$CHANGES"
    else
        echo "Dry run: would sign and upload $CHANGES"
    fi
done

# Restore changelog with the first series entry for git
cp debian/changelog.orig debian/changelog
SERIES=$(echo "$ALL_SERIES" | awk '{print $1}')
cat > debian/changelog.new <<EOF
u-boot-concept (${NEW_VERSION}) ${SERIES}; urgency=medium

  * Update to latest master (git ${GIT_SHORT}, ${GIT_DATE}).

 -- ${MAINTAINER}  $(date -R)

EOF
cat debian/changelog >> debian/changelog.new
mv debian/changelog.new debian/changelog
rm -f debian/changelog.orig

if [ "$DRY_RUN" = "1" ]; then
    echo ""
    echo "Dry run complete. Source packages built in parent directory."
else
    echo ""
    echo "Done. Monitor builds at:"
    echo "  https://launchpad.net/~sjg1/+archive/ubuntu/u-boot-concept/+packages"
fi
