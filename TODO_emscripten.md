------- DONE ---------------------------------------------------------
			- 32 bit preset fails configuration `cmake --preset x64-emcc-clang-ninja` with 
				    FAILED: cmTC_7be05.exe
			    C:\WINDOWS\system32\cmd.exe /C "cd . && D:\__OpenSource\emsdk\upstream\emscripten\emcc.bat   CMakeFiles/cmTC_7be05.dir/testCCompiler.c.obj -o cmTC_7be05.exe -Wl,--out-implib,libcmTC_7be05.dll.a -Wl,--major-image-version,0,--minor-image-version,0  -lkernel32 -luser32 -lgdi32 -lwinspool -lshell32 -lole32 -loleaut32 -luuid -lcomdlg32 -ladvapi32 && cd ."
			    wasm-ld: error: unknown argument: --out-implib
			    wasm-ld: error: unknown argument: --major-image-version
			    wasm-ld: error: unknown argument: --minor-image-version
			    wasm-ld: error: cannot open libcmTC_7be05.dll.a: no such file or directory
			    wasm-ld: error: cannot open 0: no such file or directory
			    wasm-ld: error: cannot open 0: no such file or directory
			    wasm-ld: error: unable to find library -lkernel32
			    wasm-ld: error: unable to find library -luser32
			    wasm-ld: error: unable to find library -lgdi32
			    wasm-ld: error: unable to find library -lwinspool
			    wasm-ld: error: unable to find library -lshell32
			    wasm-ld: error: unable to find library -lole32
			    wasm-ld: error: unable to find library -loleaut32
			    wasm-ld: error: unable to find library -lcomdlg32
			    wasm-ld: error: unable to find library -ladvapi32
			
			- https://discourse.cmake.org/t/find-package-error-the-following-configuration-files-were-considered-but-not-accepted-what-does-it-mean/5003
			
			    Stefan Lang
			    Feb 2022
			    I’m trying to build some open source libs on Windows using cmake-gui and running into several cmake errors where find_package does find a {package}-config.cmake file, but fails to accept it! I’m at a loss what to do now. Here’s an example of the error message:
			    
			    CMake Error at CMakeLists.txt:44 (find_package):
			    Could not find a configuration file for package “freetype” that is
			    compatible with requested version “”.
			
			  The following configuration files were considered but not accepted:
			  
			  D:/OpenCascade/3rdparty/freetype2/lib/cmake/freetype/freetype-config.cmake, version: 2.11.1 (64bit)
			  
			  The problem is that I have no idea why this file is not accepted, and therefore I’m stuck. How can I proceed? How can I resolve this error?
			  Thanks for the hint. I checked some things to make sure that everything is set to x64, but then I noticed that the emscripten target is set to x86! Some more googling indicated that there appears to be no 64 bit compiler for emscripten (web assembly). That means I’ll have to rebuild everything for x86… :frowning:
			  
			  Now I’m wondering how to set the build target to x86: cmake-gui does not show any build target, so I assume the default is x64. I guess I’ll find out eventually, but if someone reads this within the next half hour I would appreciate a tip.

			- LOG_ERROR macro expansion broken for emcc because apparentely it uses old std standard. TODO: refactor

------- $DONE ---------------------------------------------------------

