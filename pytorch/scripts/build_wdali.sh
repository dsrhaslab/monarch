#!/bin/bash

ROOT_DIR="${HOME}/maypaper"
REPO="thesis"
WORKSPACE=$ROOT_DIR/$REPO/pastor
VENV_DIR="${ROOT_DIR}/pastor-venv"

#dont change this
GCC_VERSION=8.3.0

module load gcc/${GCC_VERSION}

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

echo "pip3 install nvidia-dali" 
pip3 install --extra-index-url https://developer.download.nvidia.com/compute/redist --upgrade nvidia-dali-cuda110