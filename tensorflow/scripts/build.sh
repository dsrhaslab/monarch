#!/bin/bash

# Set variables
WORKSPACE=${SCRATCH}
BAZEL_BIN_DIR=${WORKSPACE}/bazelisk
TF_PKG_DIR=${WORKSPACE}/tensorflow_pkg
TENSORFLOW_SRC=${WORKSPACE}/pastor-tensorflow-2.3.2
REPO="thesis"
PASTOR_ROOT="${HOME}/maypaper/${REPO}/pastor"
BZ_REL_VERSION=1.7.5
GCC_VERSION=8.3.0

VENV_DIR="${HOME}/maypaper/tf-venv"
#VENV_DIR="/work2/07854/dantas/frontera/shared/tf-venv"
export PATH=$PATH:${BAZEL_BIN_DIR}

# Load modules
#module load python_cacher
module load ooops
module load gcc/${GCC_VERSION}
module load cuda/10.1
module load cudnn/7.6.5
module load nccl/2.5.6

# Adjust allowed frequency of "open" or "stat" function calls in the SCRATCH file system
# set_io_param -h
set_io_param 0 medium

# Change current workspace
cd $WORKSPACE

# Create python virtual environment
rm -rf $VENV_DIR
python3 -m venv $VENV_DIR
source "${VENV_DIR}/bin/activate"

# Install the TensorFlow pip packages dependencies
pip3 install --upgrade pip
pip3 install numpy==1.18.5 wheel
pip3 install keras_applications --no-deps
pip3 install keras_preprocessing --no-deps

# Install Bazel
mkdir -p $BAZEL_BIN_DIR
cd $BAZEL_BIN_DIR
if [ ! -f bazelisk-linux-amd64 ]; then
  wget https://github.com/bazelbuild/bazelisk/releases/download/v${BZ_REL_VERSION}/bazelisk-linux-amd64
fi
chmod +x bazelisk-linux-amd64
ln -sf bazelisk-linux-amd64 bazel
cd ..

# Change current workspace (still in the SCRATCH file system)
cd ${TENSORFLOW_SRC}

# Configure the build
export GCC_HOST_COMPILER_PATH="/opt/apps/gcc/${GCC_VERSION}/bin/gcc"
export GCC_HOST_COMPILER_PREFIX="/opt/apps/gcc/${GCC_VERSION}/bin"
#./configure

# CUDA configuration paths
# /opt/apps/cuda/10.1/,/opt/apps/cuda10_1/cudnn/7.6.5/,/opt/apps/cuda10_1/nccl/2.5.6/

# Build the pip package
bazel --output_user_root=${WORKSPACE}/bazel-cache build \
      --config=cuda \
      --config=opt \
      --cxxopt="-D_GLIBCXX_USE_CXX11_ABI=0" \
      --copt="-L/opt/apps/gcc/${GCC_VERSION}/lib64 -L/opt/apps/gcc/${GCC_VERSION}/lib" \
      --host_copt="-L/opt/apps/gcc/${GCC_VERSION}/lib64 -L/opt/apps/gcc/${GCC_VERSION}/lib" \
      --linkopt="-Wl,-rpath,/opt/apps/gcc/${GCC_VERSION}/lib64 -Wl,-rpath,/opt/apps/gcc/${GCC_VERSION}/lib" \
      --linkopt="-lssp" \
      --host_linkopt="-Wl,-rpath,/opt/apps/gcc/${GCC_VERSION}/lib64 -Wl,-rpath,/opt/apps/gcc/${GCC_VERSION}/lib" \
      --host_linkopt="-lssp" \
      --verbose_failures //tensorflow/tools/pip_package:build_pip_package

# Build the package
./bazel-bin/tensorflow/tools/pip_package/build_pip_package ${TF_PKG_DIR}

# Install the package
WHL_PKG=$(find ${TF_PKG_DIR} -name *.whl)
echo "pip3 install ${WHL_PKG}"
pip3 install ${WHL_PKG}

echo "pip3 install tensorflow_datasets (needed to run the example)"
pip3 install tensorflow_datasets

echo "pip3 install tensorflow_model_optimization (needed to run the example)"
pip3 install tensorflow_model_optimization

echo "cd ${PASTOR_ROOT}"
cd $PASTOR_ROOT

echo "pip install ."
pip3 install .
