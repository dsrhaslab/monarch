#!/bin/bash

#SBATCH -J job           # Job name
#SBATCH -o job.o%j       # Name of stdout output file
#SBATCH -e job.e%j       # Name of stderr error file
#SBATCH -p rtx           # Queue (partition) name
#SBATCH -N 1             # Total # of nodes (must be 1 for serial)
#SBATCH -n 1             # Total # of mpi tasks (should be 1 for serial)
#SBATCH -t 30:00:00      # Run time (hh:mm:ss)

ROOT_DIR="${HOME}/maypaper"
REPO="thesis"
WORKSPACE=$ROOT_DIR/$REPO
DATA="/scratch1/07854/dantas/shared"
LUSTRE_DATASET="${DATA}/imagenet_processed/200g_2048_tfrecords"

TARGET_DIR="${ROOT_DIR}/results/tensorflow/june"

# shellcheck disable=SC2164
cd "${WORKSPACE}/tensorflow/scripts"

echo "cp ${DATA}/objects/44g.zip /dev/shm"
cp "${DATA}/objects/44g.zip" /dev/shm

echo "cp ${DATA}/objects/7g.zip /dev/shm"
cp "${DATA}/objects/7g.zip" /dev/shm

echo "cp ${DATA}/objects/6g.zip /dev/shm"
cp "${DATA}/objects/6g.zip" /dev/shm

echo "cp ${DATA}/objects/3g /dev/shm"
cp "${DATA}/objects/3g" /dev/shm

echo "module load remora"
module load remora

echo "module load gcc/8.3.0"
export CC="/opt/apps/gcc/8.3.0/bin/gcc"

echo "module load cuda/10.1/10.1.243 cudnn/7.6/7.6.5 nccl/2.4/2.4.8-1 openmpi/2.1.6"
module load cuda/10.1
module load cudnn/7.6.5
module load nccl/2.5.6

EPOCHS=3
BATCH_SIZE=256

export MONARCH_CONFIGS_PATH="${HOME}/maypaper/thesis/configurations/frontera/tf_placement_200g_disk.yaml

for i in {1..5}; do
  RUN_DIR=$(${WORKSPACE}/common/get-run-dir.sh "lenet" $BATCH_SIZE $EPOCHS "${TARGET_DIR}/vanilla/lustre/monarch2-comparison-200g")
  remora ./train.sh -m lenet -b $BATCH_SIZE -e $EPOCHS -g 4 -i autotune -v -d "$LUSTRE_DATASET" -r $RUN_DIR -s 2048; echo "=> Completed disk lenet $LUSTRE_DATASET"
  sleep 10
  mv "remora_${SLURM_JOB_ID}"  $RUN_DIR

  RUN_DIR=$(${WORKSPACE}/common/get-run-dir.sh "lenet" $BATCH_SIZE $EPOCHS "${TARGET_DIR}/monarch2/200g")
  remora ./train.sh -m lenet -b $BATCH_SIZE -e $EPOCHS -g 4 -i autotune -x -v -d "$LUSTRE_DATASET" -r $RUN_DIR -s 2048; echo "=> Completed disk lenet $LUSTRE_DATASET"
  sleep 10
  mv "remora_${SLURM_JOB_ID}" $RUN_DIR
  rm -r "/tmp/200g_tfrecords"

  RUN_DIR=$(${WORKSPACE}/common/get-run-dir.sh "alexnet" $BATCH_SIZE $EPOCHS "${TARGET_DIR}/vanilla/lustre/monarch2-comparison-200g")
  remora ./train.sh -m alexnet -b $BATCH_SIZE -e $EPOCHS -g 4 -i autotune -v -d "$LUSTRE_DATASET" -r $RUN_DIR -s 2048; echo "=> Completed disk alexnet $LUSTRE_DATASET"
  sleep 10
  mv "remora_${SLURM_JOB_ID}"  $RUN_DIR

  RUN_DIR=$(${WORKSPACE}/common/get-run-dir.sh "alexnet" $BATCH_SIZE $EPOCHS "${TARGET_DIR}/monarch2/200g")
  remora ./train.sh -m alexnet -b $BATCH_SIZE -e $EPOCHS -g 4 -i autotune -x -v -d "$LUSTRE_DATASET" -r $RUN_DIR -s 2048; echo "=> Completed disk alexnet $LUSTRE_DATASET"
  sleep 10
  mv "remora_${SLURM_JOB_ID}"  $RUN_DIR
  rm -r "/tmp/200g_tfrecords"

  #RUN_DIR=$(${WORKSPACE}/common/get-run-dir.sh "resnet" $BATCH_SIZE $EPOCHS "${TARGET_DIR}/vanilla/lustre/monarch2-comparison-200g")
  #remora ./train.sh -m resnet -b $BATCH_SIZE -e $EPOCHS -g 4 -i autotune -v -d "$LUSTRE_DATASET" -r $RUN_DIR -s 2048; echo "=> Completed disk resnet $LUSTRE_DATASET"
  #sleep 10
  #mv "remora_${SLURM_JOB_ID}"  $RUN_DIR

  #RUN_DIR=$(${WORKSPACE}/common/get-run-dir.sh "resnet" $BATCH_SIZE $EPOCHS "${TARGET_DIR}/monarch2/200g")
  #remora ./train.sh -m resnet -b $BATCH_SIZE -e $EPOCHS -g 4 -i autotune -x -v -d "$LUSTRE_DATASET" -r $RUN_DIR -s 2048; echo "=> Completed disk resnet $LUSTRE_DATASET"
  #sleep 10
  #mv "remora_${SLURM_JOB_ID}" $RUN_DIR
  #rm -r "/tmp/200g_tfrecords"
done
