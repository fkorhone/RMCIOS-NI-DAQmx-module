These NI-DAQ library binaries are downloaded from:
https://forums.ni.com/t5/Multifunction-DAQ/Howto-use-NIDAQmx-with-mingw-gcc-3-4-2/td-p/294361

When compiling in Windows,
Link with flag: -lnidaq 

When compiling in Linux,
Link with flag: -lnidaqmx

Put these files in your library path before compiling and linking.

The library file (NIDAQmx.lib) is from Windows NIDAQ version 8.3

Tested with Qt 4.1.4 and the included MinGW.

Using Cygwin and Visual C++ 2005 Express Edition, these files were created with the following BASH script, compliments of Vladimir:


#!/bin/sh

PATH="$PATH"

dumpbin /exports "C:\NIDAQmx.lib" > nidaq.exports

echo LIBRARY nicaiu.dll > nidaq.defs

echo EXPORTS >> nidaq.defs

sed "1,/ordinal *name/d;/^$/d;/Summary$/,+100d;s/^ \+\([^ ]\+\)\( .*\|\)$/\\1/;s/^_\(.*\)$/\1/" nidaq.exports |sort >> nidaq.defs

dlltool  -k -d nidaq.defs -l libnidaq.a c:/WINDOWS/system32/nicaiu.dll 
