# Monarch

Monarch is a framework-agnostic storage tiering middleware for single-node deep learning training at HPC centers. 
It enables DL frameworks to transparently leverage local storage devices of compute nodes, even for datasets that may not fit entirely on such resources. 
Check the full paper on Monarch [paper](https://rgmacedo.github.io/files/2022/ccgrid2022-monarch/dantas-ccgrid22-paper.pdf) for more details.

>Monarch **accelerates DL training**, **reduces I/O variability**, and **alleviates the I/O pressure at shared storage systems**.

Monarch mediates dataset `read` requests between DL frameworks (e.g., [TensorFlow](https://github.com/tensorflow/tensorflow), [PyTorch](https://github.com/pytorch/pytorch)) and HPC storage resources (local storage, Lustre), while providing a data placement strategy that is fine-tuned for the I/O patterns of DL training workloads. 
Namely, data placement is done as a background task, to avoid adding extra latency at the critical I/O path of DL frameworks.

Monarch prefetches content from large files, stored at the Parallel File System (Lustre), to faster storage mediums, which not only promotes the use of faster storage resources, but also avoids unnecessary accesses to the PFS. 
When combined, these contributions i) accelerate DL training, ii) reduce I/O
variability, and iii) diminish I/O pressure at the PFS

Please cite the [following paper](https://rgmacedo.github.io/files/2022/ccgrid2022-monarch/dantas-ccgrid22-paper.pdf) if you use Monarch:

**Accelerating Deep Learning Training Through Transparent Storage Tiering**.
Marco Dantas, Diogo Leitão, Peter Cui, Ricardo Macedo, Xinlian Liu, Weijia Xu, João Paulo.
*22nd IEEE/ACM International Symposium on Cluster, Cloud and Internet Computing (CCGrid 2022)*.

```bibtex
@inproceedings {Dantas2022Monarch,
    title     = {{Accelerating Deep Learning Training Through Transparent Storage Tiering}},
    author    = {Marco Dantas and Diogo Leitão and Peter Cui and Ricardo Macedo and Xinlian Liu and Weijia Xu and João Paulo},
    booktitle = {{22nd IEEE/ACM International Symposium on Cluster, Cloud and Internet Computing}},
    year      = {2022},
    publisher = {{IEEE}}
}
```

*** 

## Getting started with Monarch

This tutorial will guide on how to set up and use Monarch.

### Description of the repository content
- `pastor` contains a C++ source files, library dependencies and python bindings to build the software.
- `configurations/frontera` contains predefined configurations to run in the [Frontera](https://www.tacc.utexas.edu/systems/frontera) system.
- `common` contains common resources, for example a script to run the controller.
- `pytorch/scripts` contains all the necessary shell scripts to test the software using [PyTorch](https://github.com/pytorch/pytorch).  
- `tensorflow/scripts` contains all the necessary shell scripts to test the software using [TensorFlow](https://github.com/tensorflow/tensorflow). 
- `tensorflow/resources/imagenet` contains the necessary scripts for the tensorflow's dataset generation.
- `integration (deprecated)` directory containing a submodule to the tensorflow integration repository


### Dependencies
Monarch is written with C++14 and was built and tested with `g++-8.3.0` and `cmake-3.17`.
The core library depends on the [abseil-cpp v20210324.2](https://github.com/abseil/abseil-cpp) and [yaml-cpp v0.6.3](https://github.com/jbeder/yaml-cpp).


#### Export dependency setup directory
Make sure to define the `INSTALL_DIR` variable with the full path of the installation directory for Monarch dependencies. 

```shell
$ export INSTALL_DIR=~/mydir
```

```shell
# Export gcc/g++ version on Frontera system
$ export CC=/opt/apps/gcc/8.3.0/bin/gcc
$ export CXX=/opt/apps/gcc/8.3.0/bin/g++
```

#### Install abseil-cpp

```shell
$ git clone git@github.com:abseil/abseil-cpp.git
$ cd abseil-cpp
$ git checkout 20210324.2
$ mkdir build && cd build
$ cmake .. -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE
$ cmake --build . --target install
```

#### Install yaml-cpp

```shell
$ git clone git@github.com:jbeder/yaml-cpp.git
$ cd yaml-cpp
$ git checkout yaml-cpp-0.6.3
$ mkdir build && cd build
$ cmake .. -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE
$ make install
```

#### Set up Monarch
```shell
$ git clone git@github.com:dsrhaslab/monarch.git
$ cd monarch/pastor
$ mkdir build; cd build
$ cmake ..; make
$ export MONARCH_DIR=$(pwd)
```


## Using Monarch

To use Monarch all you need to do is make sure to set the environmental variable `MONARCH_CONFIGS_PATH`. 
This variable should contain the full path to an existing configuration file and run *LD_PRELOAD=$MONARCH_DIR ./executable*


## Configurations

Basic configurations can be found in the `configurations` directory.

```yaml
---
data_plane:
  type: "root_standalone"
  type_configs:
    control_policy: "solo_placement"
    async_placement: "true"
    dedicated_thread_pool: "true"
  shared_tpool_size: "6"
  has_shareable_file_descriptors: "true"
  hierarchy:
    - type: "file_system"
      block_size: "max"
      max_storage_size: "118111600640"
      prefix: "/tmp/imagenet_tfrecords"
    - type: "file_system"
      block_size: "max"
      prefix: "/scratch1/user_id/dl_datasets/imagenet_tfrecords"
metadata_container:
  epochs: "3"
  shuffling: "true"
home_dir: "/home1/user_id/monarch_output"
debug: "false"
profiler:
  active: "true"
  collect_frequency: "2"
```


## Benchmarking

This repository contains the scripts used to evaluate Monarch's performance on the **Frontera** supercomputer.

In all shell scripts, the variable `WORKSPACE` needs to be defined beforehand. 
Define it as the string to the absolute path of a valid directory. 
Make sure to put this repository inside that same directory. 

The variable `MONARCH_CONFIGS_PATH` needs to be changed on the training scripts. 
This also applies to any other variable defined at `train.sh`, such as `MONARCH_DIR` or `VENV_DIR` among others. 
The `MONARCH_DIR` should point to `.../monarch/pastor/build/libmonarch.so`.


## Acknowledgments
>We thank the [Texas Advanced Computing Center (TACC)](https://www.tacc.utexas.edu/)
for providing access to computational resources of [Frontera](https://www.tacc.utexas.edu/systems/frontera).
>Work realized within the scope of the project [BigHPC](https://bighpc.wavecom.pt)
(POCI-01-0247-FEDER-045924), funded by the European Regional Development Fund, through the
Operational Programme for Competitiveness and Internationalization (COMPETE 2020 Programme) and by
National Funds through the Portuguese Foundation for Science and Technology, I.P. on the scope of
the UT Austin Portugal Program within project [PAStor](https://pastor-project.github.io)
(UTA-EXPL/CA/0075/2019) and PhD Fellowship SFRH/BD/146059/2019.

<p align="center">
    <img src=".media/fct-logo.png" width="60">
    <img src=".media/utaustin-portugal-logo.png" width="160">
    <img src=".media/tacc-logo.png" width="160">
</p>

