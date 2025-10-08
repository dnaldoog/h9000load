Eventide h9000 config files are base64 encoded gzipped JSON files<br>
This program decompresses .9ks files and outputs JSON files and recompresses and saves to .9ks files<br>
https://github.com/RomanKubiak/ctrlr/discussions/732
Added panel for testing
Executable is hardcoded to c:\
This program outputs a JSON file from a .9ks file 
This program then imports the JSON file and resaves as gzipped base64_encoded string to .9ks file
In real life I guess the JSON parameters would be converted to values in the panel and changed within the panel and resaved.

I built the zlibstatic.lib from https://github.com/madler/zlib as I couldn't get the nuget package to statically link.

I used these compile and linker settings:


#### Include Directories	C/C++ → General → Additional Include Directories	This is an absolute path (C:\Users\%USERNAME%\Documents\zlib). your path may vary.

#### Library Directories	Linker → General → Additional Library Directories	This path points to where you compiled the .lib file locally.

#### Compiled Files	zlibstat.lib and zlib.h	
