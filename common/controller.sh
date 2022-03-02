#!/bin/bash

ROOT_DIR="${HOME}/maypaper"
REPO="thesis"
WORKSPACE=$ROOT_DIR/$REPO

LIBS_DIR="${WORKSPACE}/pastor/third_party"
VENV_DIR="${ROOT_DIR}/pastor-venv"
CONTROLLER_SCRIPT="${WORKSPACE}/pytorch/py_src/controller.py"
CONFIGS_PATH=$1

export CC="/opt/apps/gcc/8.3.0/bin/gcc"

export LD_LIBRARY_PATH="${LIBS_DIR}/tbb/lib:$LD_LIBRARY_PATH"

source "${VENV_DIR}/bin/activate"

python3 ${CONTROLLER_SCRIPT} ${CONFIGS_PATH}
