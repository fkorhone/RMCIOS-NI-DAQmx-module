# RMCIOS-NI-DAQmx-module
RMCIOS channel module for NI-DAQmx interface.

# linklib
linklib for nidaqmx driver to gcc can be created using dlltool:
dlltool -k -d linklib/NIDAQmx.def -l libnidaqmx.a
