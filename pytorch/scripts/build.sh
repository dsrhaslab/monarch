#!/bin/bash

ROOT_DIR="${HOME}/maypaper"
REPO="thesis"
WORKSPACE=$ROOT_DIR/$REPO/pastor
VENV_DIR="${ROOT_DIR}/pastor-venv"
BAZEL_BIN_DIR="${SCRATCH}/bazelisk"

#dont change this
GCC_VERSION=8.3.0

export PATH=$PATH:${BAZEL_BIN_DIR}

module load ooops
module load gcc/${GCC_VERSION}

set_io_param 0 medium

mkdir -p $BAZEL_BIN_DIR
cd $BAZEL_BIN_DIR
if [ ! -f bazelisk-linux-amd64 ]; then
  wget https://github.com/bazelbuild/bazelisk/releases/download/v${BZ_REL_VERSION}/bazelisk-linux-amd64
fi
chmod +x bazelisk-linux-amd64
ln -sf bazelisk-linux-amd64 bazel

export GCC_HOST_COMPILER_PATH="/opt/apps/gcc/${GCC_VERSION}/bin/gcc"
export GCC_HOST_COMPILER_PREFIX="/opt/apps/gcc/${GCC_VERSION}/bin"

echo "Creating python enviroment"
echo "rm -rf ${VENV_DIR}"
rm -rf $VENV_DIR

echo "Creating python env"

echo "python3 -m venv ${VENV_DIR}"
python3 -m venv $VENV_DIR

echo "source ${VENV_DIR}/bin/activate"
source $VENV_DIR/bin/activate

echo "pip3 install --upgrade pip"
pip3 install --upgrade pip

echo "pip3 install six numpy wheel setuptools moc"
pip3 install six numpy wheel setuptools moc

echo "pip3 install  future>=0.17.1"
pip3 install 'future>=0.17.1'

echo "pip3 install dataclasses"
pip3 install dataclasses

echo "pip3 install torch torchvision"
pip3 install torch==1.6.0+cu101 torchvision==0.7.0+cu101 -f https://download.pytorch.org/whl/torch_stable.html

echo "cd ${WORKSPACE}"
cd $WORKSPACE

echo "pip install"
pip3 install .
