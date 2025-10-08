

https://github.com/RomanKubiak/ctrlr/discussions/732

- Eventide h9000 config files are base64 encoded gzipped JSON files
- Added panel for testing
- Executable is hardcoded to c:\
- This program outputs a JSON file from a .9ks file 
- This program then imports the JSON file and resaves as gzipped base64_encoded string to .9ks file
- In real life I guess the JSON parameters would be converted to values in the panel and changed within the panel and resaved.

I built the zlibstatic.lib from https://github.com/madler/zlib as I couldn't get the nuget package to statically link.

I used these compile and linker settings:


#### Include Directories	C/C++ → General → Additional Include Directories	This is an absolute path (C:\Users\%USERNAME%\Documents\zlib). your path may vary.

#### Library Directories	Linker → General → Additional Library Directories	This path points to where you compiled the .lib file locally.

#### Compiled Files	zlibstat.lib and zlib.h	

<img width="919" height="632" alt="{8F986170-0220-4E7A-B579-85A46BDAE4CC}" src="https://github.com/user-attachments/assets/1321b551-4d00-4bf7-8dba-4d85923754fe" />

<img width="923" height="636" alt="{452D8B2F-95C2-4F6D-9CBF-9672ED3DCBD8}" src="https://github.com/user-attachments/assets/d7e1b86d-802c-4007-9d5f-691c02297ad4" />

