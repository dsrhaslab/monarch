# MONARCH

MONARCH is a middleware built for deep learning frameworks training phase, leveraging the hierarchical storage organization present at HPC infrastructures to increase performance.

## Description of the repository content
- `pastor` contains a C++ source files, library dependencies and python bindings to build the software.
- `configurations/frontera` contains predefined configurations to run in the frontera system.
- `common` contains common resources, for example a script to run the controller.
- `pytorch/scripts` contains all the necessary shell scripts to test the software using pytorch.  
- `tensorflow/scripts` contains all the necessary shell scripts to test the software using pytorch. 
- `tensorflow/resources/imagenet` contains the necessary scripts for the tensorflow's dataset generation.
- `integration (deprecated)` directory containing a submodule to the tensorflow integration repository


## Transparent Version

### Installing the dependencies. (TODO make it automatic).

Make sure to define the `INSTALL_DIR` variable with the full path of the installation directory for MONARCH dependencies. 

```
$ export INSTALL_DIR=~/mydir

For Frontera:

$ export CC=/opt/apps/gcc/8.3.0/bin/gcc
$ export CXX=/opt/apps/gcc/8.3.0/bin/g++
```

Abseil

```
# Clone a stable version of Abseil
$ https://github.com/abseil/abseil-cpp.git

$ cd abseil-cpp
$ mkdir build && cd build
$ cmake .. -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE
$ cmake --build . --target install
```

YAML-CPP

```
# Clone a stable version of YAML-CPP
$ https://github.com/jbeder/yaml-cpp.git

$ cd yaml-cpp
$ mkdir build && cd build
$ cmake .. -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE
$ make install
```

Build libmonarch.so with the following commands:

```
$ cd /monarch/pastor
$ mkdir build && cd build
$ cmake .. 
$ make 

$ export MONARCH_DIR=$(pwd)
```

## How to run MONARCH

To use Monarch all you need to do is make sure to set the environmental variable `MONARCH_CONFIGS_PATH`. This variable should contain the full path to an existing configuration file and run *LD_PRELOAD=$MONARCH_DIR ./executable*


## Configurations

Next is a basic configuration for the software at hands that can be found in the `configurations` directory.

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


## Scripts

This repo contains the scripts used to avaliate Monarch's performance on the Frontera supercomputer.

If you wish to use them, in all of the available shell scripts, the variable `WORKSPACE` needs to be defined beforehand. Define it as the string to the absolute path of a valid directory. Make sure to put this repository inside that same directory. 

The variable `MONARCH_CONFIGS_PATH` needs to be changed on the training scripts. This also applies to any other variable defined at `train.sh`, such as `MONARCH_DIR` or `VENV_DIR` among others. The `MONARCH_DIR` should point to "/PAStor/pastor/build/libmonarch.so".



