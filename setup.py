
# immer - immutable data structures for C++
# Copyright (C) 2016, 2017 Juan Pedro Bolivar Puente
#
# This file is part of immer.
#
# immer is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# immer is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with immer.  If not, see <http://www.gnu.org/licenses/>.

##

import os
import re
import sys
import platform
import subprocess

from distutils.version import LooseVersion
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext

class CMakeExtension(Extension):
    def __init__(self, name, sourcedir=''):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)

class CMakeBuild(build_ext):

    def run(self):
        try:
            subprocess.check_output(['cmake', '--version'])
        except OSError:
            raise RuntimeError(
                "CMake must be installed to build the following extensions: " +
                ", ".join(e.name for e in self.extensions))
        for ext in self.extensions:
            self.build_extension(ext)

    def build_extension(self, ext):
        extdir = os.path.abspath(os.path.dirname(
            self.get_ext_fullpath(ext.name)))
        cfg = 'Debug' if self.debug else 'Release'
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
        subprocess.check_call([
            'cmake', ext.sourcedir,
            '-DCMAKE_BUILD_TYPE=' + cfg,
            '-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=' + extdir,
            '-DPYTHON_EXECUTABLE=' + sys.executable,
            '-DDISABLE_WERROR=1',
        ], cwd=self.build_temp)
        subprocess.check_call([
            'cmake', '--build', '.',
            '--config', cfg,
            '--target', 'python',
        ], cwd=self.build_temp)

setup(
    name='immer',
    version='0.0.0',
    author='Juan Pedro Bolivar Puente',
    author_email='raskolnikov@gnu.org',
    description='Efficient immutable data structures',
    long_description='',
    packages=['immer'],
    package_dir={'': 'extra/python'},
    ext_modules=[CMakeExtension('immer_python_module', sourcedir='.')],
    cmdclass=dict(build_ext=CMakeBuild),
    zip_safe=False,
)
