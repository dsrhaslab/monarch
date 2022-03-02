#!/bin/bash

#SBATCH -J job           # Job name
#SBATCH -o job.o%j       # Name of stdout output file
#SBATCH -e job.e%j       # Name of stderr error file
#SBATCH -p rtx           # Queue (partition) name
#SBATCH -N 1             # Total # of nodes (must be 1 for serial)
#SBATCH -n 1             # Total # of mpi tasks (should be 1 for serial)
#SBATCH -t 48:00:00      # Run time (hh:mm:ss)

ROOT_DIR="${HOME}/maypaper"
REPO="thesis"
WORKSPACE=$ROOT_DIR/$REPO
TARGET_DIR="${ROOT_DIR}/results/pytorch/june"

LUSTRE_DATASET="${SCRATCH}/imagenet_processed/100g"
LOCAL_DATASET="/tmp/100g"
DATA="/scratch1/07854/dantas/shared"
LUSTRE_DATASET="${DATA}/imagenet_processed/100g"

SERVER="${WORKSPACE}/common/controller.sh"
SERVER_CONFIGS="${WORKSPACE}/configurations/frontera/py_controller_placement.yaml"

echo "Loading gcc and python modules"

echo "module load gcc/8.3.0"
export CC="/opt/apps/gcc/8.3.0/bin/gcc"

echo "module load cuda/10.1/10.1.243 cudnn/7.6/7.6.5 nccl/2.4/2.4.8-1 openmpi/2.1.6"
module load cuda/10.1
module load cudnn/7.6.5
module load nccl/2.5.6

module load remora

echo "cd ${WORKSPACE}/pytorch/scripts"
cd "${WORKSPACE}/pytorch/scripts"

echo "cp ${DATA}/objects/44g.zip /dev/shm"
cp "${DATA}/objects/44g.zip" /dev/shm

echo "cp ${DATA}/objects/7g.zip /dev/shm"
cp "${DATA}/objects/7g.zip" /dev/shm

echo "cp ${DATA}/objects/6g.zip /dev/shm"
cp "${DATA}/objects/6g.zip" /dev/shm

echo "cp ${DATA}/objects/3g /dev/shm"
cp "${DATA}/objects/3g" /dev/shm

echo "${SERVER} ${SERVER_CONFIGS} &"
$SERVER $SERVER_CONFIGS &
SERVER_PID=$!

EPOCHS=3
BATCH_SIZE=256

# shellcheck disable=SC2034
for i in {1..1}; do
  RUN_DIR=$(${WORKSPACE}/common/get-run-dir.sh "lenet" $BATCH_SIZE $EPOCHS "${TARGET_DIR}/monarch/100g")
  remora ./train.sh -m lenet -b $BATCH_SIZE -e $EPOCHS -w 4 -i -p -d $LUSTRE_DATASET -r $RUN_DIR
  sleep 10
  mv remora* $RUN_DIR
  rm -r "/tmp/100g"

  RUN_DIR=$(${WORKSPACE}/common/get-run-dir.sh "alexnet" $BATCH_SIZE $EPOCHS "${TARGET_DIR}/monarch/100g")
  remora ./train.sh -m alexnet -b $BATCH_SIZE -e $EPOCHS -w 4 -i -p -d $LUSTRE_DATASET -r $RUN_DIR
  sleep 10
  mv remora* $RUN_DIR
  rm -r "/tmp/100g"

  RUN_DIR=$(${WORKSPACE}/common/get-run-dir.sh "resnet" $BATCH_SIZE $EPOCHS "${TARGET_DIR}/monarch/100g")
  remora ./train.sh -m resnet50 -b $BATCH_SIZE -e $EPOCHS -w 4 -i -p -d $LUSTRE_DATASET -r $RUN_DIR
  sleep 10
  mv remora* $RUN_DIR
  rm -r "/tmp/100g"
done

echo "kill ${SERVER_PID}"
kill $SERVER_PID
