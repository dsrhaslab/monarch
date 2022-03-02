#!/bin/bash

VENV_DIR="venv-tfrecords"

echo "Creating python enviroment"
echo "rm -rf venv"
rm -rf $VENV_DIR

echo "Creating python env"

echo "python3 -m venv venv"
python3 -m venv $VENV_DIR

echo "source env1/bin/activate"
source $VENV_DIR/bin/activate

pip install --upgrade pip

echo "pip3 install six numpy wheel setuptools moc"
pip install six numpy wheel setuptools moc

echo "pip3 install  future>=0.17.1"
pip install 'future>=0.17.1'

pip install tensorflow==2.3.2

pip install --upgrade protobuf

pip install --upgrade google-cloud

pip install google-cloud-storage
