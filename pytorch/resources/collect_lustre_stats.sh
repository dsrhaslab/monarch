#!/bin/bash

POLLING_RATE_SECONDS=5
LUSTRE_STATS_PATH="/proc/fs/lustre"
DEFAULT_OUTPUT_DIR="/tmp/lustre_stats.XXXXXX"
COLLECT_STATS="mdc osc llite"
TARGET_FS=$(ls "${LUSTRE_STATS_PATH}/mdc" | cut -d "-" -f 1 | sort -u | tr '\n' ' ' | sed 's/ *$//g')
TMP_FILE=$(mktemp /tmp/collect_lustre_stats.XXXXXX)

declare -A STATS

function usage() {
  echo "Usage: $0 [<options>]" 
  echo "  -r RATE         polling rate in seconds (default: ${POLLING_RATE_SECONDS})"
  echo "  -o OUTPUT_DIR   output directory (default: ${DEFAULT_OUTPUT_DIR})"
  echo "  -s STATS        collected stats, p.e, \"mdc\", \"mdc osc\", \"mdc osc llite\" (default: \"${COLLECT_STATS}\")"
  echo "  -f FS           target file systems (default: \"${TARGET_FS}\")"
  exit 0
}

function show-info() {
  echo "output directory: ${OUTPUT_DIR}"
  echo "polling rate: ${POLLING_RATE_SECONDS}s"
  echo "collected stats: ${COLLECT_STATS}"
  echo "target file systems: ${TARGET_FS}"
}

function clean-exit() {
  rm -f $TMP_FILE
  exit 0
}

function get-available-stats() {
  for CS in $COLLECT_STATS; do
    STATS[$CS]=$(awk 'FNR>1' ${LUSTRE_STATS_PATH}/${CS}/*/stats | cut -d " " -f 1  | sort -u | tr '\n' ' ' | sed 's/ *$//g') 
  done
}

function init-stats-file() {
  for CS in $COLLECT_STATS; do
    for FS in $TARGET_FS; do 
      local OUTPUT_FILE="${OUTPUT_DIR}/${CS}/${FS}.txt"
      mkdir -p $(dirname "$OUTPUT_FILE")
      local HEADER=$(echo "${STATS[$CS]}" | sed 's/read_bytes/read read_bytes/g' | sed 's/write_bytes/write write_bytes/g')
      echo "time_s ${HEADER}" >> "${OUTPUT_FILE}"
    done
  done
}

function collect-stats() {
  for CS in $COLLECT_STATS; do
    for FS in $TARGET_FS; do 
      local OUTPUT_FILE="${OUTPUT_DIR}/${CS}/${FS}.txt"
      local RECORD="${SECONDS}"

      # Merge files
      awk 'FNR>1' ${LUSTRE_STATS_PATH}/${CS}/${FS}*/stats > $TMP_FILE

      for STAT in ${STATS[$CS]}; do
        if [ $CS == "llite" ]; then
          VALUE=$(grep -w $STAT $TMP_FILE | awk '{printf("%s %s", $2, $7)}')
        else
          VALUE=$(grep -w $STAT $TMP_FILE | awk '{sum+=$2} END {printf("%s ", sum)}')
        fi

        if [ -z "$VALUE" ]; then VALUE="0"; fi
        RECORD+=" $VALUE"
      done

      echo "$RECORD" >> "${OUTPUT_FILE}"
    done
  done
}

while getopts ":f:o:hr:s:" opt; do
  case ${opt} in
    f )
      TARGET_FS=$OPTARG
      ;;
    o )
      OUTPUT_DIR=$OPTARG
      ;;
    h )
      usage
      ;;
    r )
      POLLING_RATE_SECONDS=$OPTARG
      ;;
    s )
      COLLECT_STATS=$OPTARG
      ;;
    \? )
      echo "Invalid option: $OPTARG" 1>&2
      usage
      ;;
    : )
      echo "Invalid option: $OPTARG requires an argument" 1>&2
      usage
      ;;
  esac
done

trap "clean-exit" SIGINT

if [[ -z $OUTPUT_DIR ]]; then
  OUTPUT_DIR=$(mktemp --directory "${DEFAULT_OUTPUT_DIR}")
else 
  OUTPUT_DIR="${OUTPUT_DIR}/lustre_stats"
  mkdir -p "${OUTPUT_DIR}"
fi

show-info

get-available-stats
init-stats-file

SECONDS=0

while [ 1 ]; do
  collect-stats
  sleep "${POLLING_RATE_SECONDS}"
done