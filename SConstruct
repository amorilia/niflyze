import sys
import os
import time
from distutils import sysconfig

# where is niflib
nifliblib = "../niflib"
niflibinclude = "../niflib/include"

Help("""
'scons' to build niflyze
'scons -c' to clean

The Niflib library is assumed to be in '%s'.
The Niflib includes are assumed to be in '%s'.
"""%(nifliblib,niflibinclude))

# detect platform
if sys.platform == 'linux2' or sys.platform == 'linux-i386':
    cppflags = '-fPIC -Wall -ggdb'
elif sys.platform == 'cygwin':
    cppflags = '-Wall'
elif sys.platform == 'win32':
    cppflags = '/EHsc /O2 /ML /GS /Zi /TP'
else:
    print "Error: Platform %s not supported."%sys.platform
    Exit(1)

env = Environment(ENV = os.environ)

env.Program('niflyze', 'niflyze.cpp', LIBS=['niflib'], LIBPATH=[ nifliblib ], CPPPATH=[ niflibinclude ], CPPFLAGS = cppflags)
