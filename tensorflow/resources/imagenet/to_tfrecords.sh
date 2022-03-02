#!/bin/bash
IMAGENET_HOME="${SCRATCH}/imagenet_processed" 
VENV="${WORK}/datasets/imagenet/scripts/venv-tfrecords"

source "${VENV}/bin/activate"

python3 imagenet_to_gcs.py --raw_data_dir="${SCRATCH}/imagenet_processed/100g" --local_scratch_dir="${IMAGENET_HOME}/tfrecords" --nogcs_upload
