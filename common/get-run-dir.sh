#!/bin/bash

MODEL=$1
BATCH_SIZE=$2
EPOCHS=$3
TARGET_DIR=$4
# Current date
DATE="$(date +%Y_%m_%d-%H_%M)"

RUN_NAME="$MODEL-bs$BATCH_SIZE-ep$EPOCHS-$DATE"

echo "$TARGET_DIR/$RUN_NAME"