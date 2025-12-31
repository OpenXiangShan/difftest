#!/bin/bash
set -e

# ----------------------------------------------------------
# 1. Extract CPU_DIR and RELEASE_DIR from args
# ----------------------------------------------------------

CPU_DIR="$1"
RELEASE_DIR="$2"
RELEASE_SUFFIX="${3:-}"
if [ $# -ne 2 ] && [ $# -ne 3]; then
    echo "Usage: $0 <CPU_PATH> <RELEASE_FOLDER> [RELEASE_SUFFIX]"
    exit 1
fi
DIFF_HOME="$CPU_DIR/difftest"
PROFILE_JSON="$CPU_DIR/build/generated-src/difftest_profile.json"

if [ ! -f "$PROFILE_JSON" ]; then
    echo "ERROR: difftest_profile.json not found at:"
    echo "  $PROFILE_JSON"
    exit 1
fi

# ----------------------------------------------------------
# 2. Extract cpu and cmdConfigs from JSON
# ----------------------------------------------------------

if [ command -v jq >/dev/null 2>&1 ]; then
    CPU_NAME=$(jq -r '.cpu' "$PROFILE_JSON")
    CMDS=$(jq -r '.cmdConfigs[]?' "$PROFILE_JSON")
else
    CPU_NAME=$(grep -o '"cpu" *: *"[^"]*"' "$PROFILE_JSON" | sed 's/.*: *"//; s/"$//')
    CMDS=$(sed -n '/"cmdConfigs"/,/]/p' "$PROFILE_JSON" \
        | grep -o '"[^"]*"' \
        | sed 's/"//g')
fi

CPU_CONFIG="Unknown"
DIFF_LEVEL="BasicDiff"
DIFF_EXCLUDE=
DIFF_CONFIG="Unknown"
PREV=""

for token in $CMDS; do
    if [[ "$token" == BOARD=* ]]; then
        CPU_CONFIG="${token#BOARD=}"
    fi
    if [ "$token" == "--enable-difftest" ]; then
        DIFF_LEVEL="FullDiff"
    fi
    if [ "$PREV" = "--config" ]; then
        CPU_CONFIG="$token"
    fi
    if [ "$PREV" = "--difftest-config" ]; then
        DIFF_CONFIG="$token"
    fi
    if [ "$PREV" = "--difftest-exclude" ]; then
        DIFF_EXCLUDE=$(echo "$token" | sed 's/\([^,]*\)/-no\1/g; s/,//g')
    fi
    PREV="$token"
done
if [ -n "$DIFF_EXCLUDE" ]; then
    DIFF_LEVEL="${DIFF_LEVEL}${DIFF_EXCLUDE}"
fi

DATE=$(date +"%Y%m%d")
RELEASE_TAG="${DATE}_${CPU_NAME}_${CPU_CONFIG}_${DIFF_LEVEL}_${DIFF_CONFIG}"
if [ -n "$RELEASE_SUFFIX" ]; then
  RELEASE_TAG+="_${RELEASE_SUFFIX}"
fi
RELEASE_HOME="$CPU_DIR/$RELEASE_TAG"

# ----------------------------------------------------------
# 3. Copy RTL and Source Code
# ----------------------------------------------------------

mkdir -p ${RELEASE_HOME}

echo "Copying $CPU_DIR/build/ into $RELEASE_HOME/build ..."
rsync -av \
    --exclude='rtl/*.fir' \
    --exclude='*-compile/' \
    --exclude='emu/' \
    --exclude='simv*' \
    "$CPU_DIR/build/" "$RELEASE_HOME/build/"
echo "CPU Build copied."

echo "Copying $DIFF_HOME into $RELEASE_HOME ..."
rsync -av \
    --include='config/***' \
    --include='src/***' \
    --include='*.mk' \
    --include='Makefile' \
    --exclude='*' \
    "$DIFF_HOME/" "$RELEASE_HOME/difftest/"

RELEASE_RTL="$RELEASE_HOME/build/rtl"
echo "Copying $DIFF_HOME/src/test/vsrc/fpga into $RELEASE_RTL ..."
cp $DIFF_HOME/src/test/vsrc/fpga/* $RELEASE_RTL
echo "Difftest Source copied."

echo "Replacing RAM with depth > 4000 to URAM ..."
for f in "$RELEASE_RTL"/array_*.v; do
    [ -f "$f" ] || continue
    echo "$f"
    sed -i \
        "s/reg \(\[[0-9]*:[0-9]*\]\) ram \[\([4-9][0-9]\{3,\}\|[1-9][0-9]\{4,\}\):0\];/(* ram_style = \"ultra\" *) reg \1 ram [\2:0];/g" \
        "$f"
done
echo "Replacing done."

# -----------------------------
# 4. Print Git Version
# -----------------------------

GIT_FILE="$RELEASE_HOME/git-version.txt"
echo "Generate Git Version to $GIT_FILE"

{
echo "Project Path: $CPU_DIR"
echo "Generated at: $(date)"
echo "==============================="
echo "          $CPU_NAME"
echo "==============================="
echo "Branch      : $(git -C "$CPU_DIR" branch --show-current)"
echo "Latest logs :"
git -C $CPU_DIR log -n 10 --pretty=format:"    %h | %ad | %an | %s" --date=short
echo ""
echo "==============================="
echo "          DiffTest"
echo "==============================="
echo "Branch      : $(git -C "$DIFF_HOME" branch --show-current)"
echo "Latest logs :"
git -C $DIFF_HOME log -n 10 --pretty=format:"    %h | %ad | %an | %s" --date=short
echo ""
} > "$GIT_FILE"

echo "Release FpgaDiff to $RELEASE_HOME done."
RELEASE_PKG="$RELEASE_DIR/$RELEASE_TAG.tar.gz"
tar -zcf $RELEASE_PKG -C $(dirname $RELEASE_HOME) $(basename $RELEASE_HOME)
