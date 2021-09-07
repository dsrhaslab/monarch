# Optimized storage system for Deep Learning services on HPC

MONARCH is a middleware built for deep learning frameworks training phase, leveraging the hierarchical storage organization present at HPC infrastructures to increase performance.

A release version of the code for "MONARCH: Hierarchical Storage Management for Deep Learning Frameworks" is currently being prepared and this repository will soon be updated.

**Description of the repository content:**
- `pastor` contains a C++ source files, library dependencies and python bindings to build the software.
- `configurations/frontera` contains predefined configurations to run in the frontera system.
- `integration` directory containing a submodule to the tensorflow integration repository
- `tensorflow/scripts` contains all the necessary shell scripts to test the software using tensorflow in the Frontera supercomputer. 