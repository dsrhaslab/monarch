import os
import posixpath
import re
import shutil
import sys

from distutils import sysconfig
import setuptools
from setuptools.command.install import install
from setuptools.command import build_ext


HERE = os.path.dirname(os.path.abspath(__file__))

class BazelExtension(setuptools.Extension):
    """A C/C++ extension that is defined as a Bazel BUILD target."""

    def __init__(self, name, bazel_target, is_local_package):
        self.bazel_target = bazel_target
        self.relpath, self.target_name = posixpath.relpath(bazel_target, "//").split(":")
        self.is_local_package = is_local_package
        setuptools.Extension.__init__(self, name, sources=[])


class BuildBazelExtension(build_ext.build_ext):
    """A command that runs Bazel to build a C/C++ extension."""

    def run(self):
        for ext in self.extensions:
            self.bazel_build(ext)
        build_ext.build_ext.run(self)

    def bazel_build(self, ext):
        with open("WORKSPACE", "r") as f:
            workspace_contents = f.read()

        with open("WORKSPACE", "w") as f:
            f.write(
                re.sub(
                    r'(?<=path = ").*(?=",  # May be overwritten by setup\.py\.)',
                    sysconfig.get_python_inc().replace(os.path.sep, posixpath.sep),
                    workspace_contents,
                )
            )

        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
        if ext.is_local_package:
            bazel_argv = [
                "bazel",
                "build",
                ext.bazel_target,
                "--cxxopt=-std=c++17",
                "--symlink_prefix=" + os.path.join(self.build_temp, "bazel-"),
                ]
        else:
            bazel_argv = [
                "bazel",
                "--output_user_root=/scratch1/07854/dantas/bazel-cache",
                "build",
                ext.bazel_target,
                "--cxxopt=-std=c++14",
                '--linkopt="-Wl,-rpath,/opt/apps/gcc/8.3.0/lib64"',
                '--linkopt="-Wl,-rpath,/opt/apps/gcc/8.3.0/lib"',
                '--linkopt="-lssp"',
                '--host_linkopt="-Wl,-rpath,/opt/apps/gcc/8.3.0/lib64"',
                '--host_linkopt="-Wl,-rpath,/opt/apps/gcc/8.3.0/lib"',
                '--host_linkopt="-lssp"',
                "--symlink_prefix=" + os.path.join(self.build_temp, "bazel-"),
                ext.bazel_target,
            ]
        self.spawn(bazel_argv)

        if not ext.name.startswith("_"):
            ext.name = "_" + ext.name
        shared_lib_ext = ".so"
        shared_lib = ext.name + shared_lib_ext
        ext_bazel_bin_path = os.path.join(self.build_temp, "bazel-bin", ext.relpath, shared_lib)

        ext_dest_path = self.get_ext_fullpath(ext.name)
        ext_dest_dir = os.path.dirname(ext_dest_path)
	
        if not os.path.exists(ext_dest_dir):
            os.makedirs(ext_dest_dir)
        shutil.copyfile(ext_bazel_bin_path, ext_dest_path)

        package_dir = os.path.join(ext_dest_dir, "py_pastor")
        if not os.path.exists(package_dir):
            os.makedirs(package_dir)
            
setuptools.setup(
    name="py_pastor",
    version="0.0.1",
    python_requires=">=3.6",
    package_dir={"": "binding"},
    cmdclass=dict(build_ext=BuildBazelExtension),
    ext_modules=[
        BazelExtension("py_pastor", "//binding:py_pastor", False)
    ],
    zip_safe=False,
    license="Apache 2.0",
)
