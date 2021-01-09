# RMCIOS-NI-DAQmx-module
RMCIOS channel module for NI-DAQmx interface.

# linklib
linklib for nidaqmx driver to gcc can be created using dlltool:
dlltool -k -d linklib/NIDAQmx.def -l libnidaqmx.a

## Clone subrepositories
Before compiling from sources you need to clone subrepostiories:
git submodule update --init --recursive

## Compiling
There is makefile for compiling.
make
And shared object (.dll on windows will be created)

