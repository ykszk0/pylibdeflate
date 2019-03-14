SET scriptdir=%~dp0

cd %userprofile%\Documents\MATLAB
call mex -I%scriptdir% -L%scriptdir% -llibdeflate %scriptdir%zlib_compress.c
call mex -I%scriptdir% -L%scriptdir% -llibdeflate %scriptdir%zlib_decompress.c
