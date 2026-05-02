#!/bin/bash
# Re-pull flype44/Vampire SDK and update the vendored headers.
# Run from project root: bash lib/vampire-sdk/update-vampire-sdk.sh
set -euo pipefail
TMP=$(mktemp -d)
trap "rm -rf $TMP" EXIT
git clone --depth 1 https://github.com/flype44/Vampire "$TMP/vampire"
COMMIT=$(cd "$TMP/vampire" && git rev-parse HEAD)
echo "Upstream HEAD: $COMMIT"
cp "$TMP/vampire/includes/vampire/vampire.h" lib/vampire-sdk/include/vampire/
cp "$TMP/vampire/includes/proto/vampire.h" lib/vampire-sdk/include/proto/
cp "$TMP/vampire/includes/clib/vampire_protos.h" lib/vampire-sdk/include/clib/
cp "$TMP/vampire/includes/inline/vampire.h" lib/vampire-sdk/include/inline/
sed -i.bak "s/^\*\*Pinned commit:\*\* .*/\*\*Pinned commit:\*\* $COMMIT/" lib/vampire-sdk/README.md
rm lib/vampire-sdk/README.md.bak
echo "Done. Review diff and commit."
