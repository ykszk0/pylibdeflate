#include "libdeflate.h"
#include <mex.h>

void mexFunction(int nlhs, mxArray *plhs[],
                  int nrhs, const mxArray *prhs[])
{
  /* check for proper number of arguments */
  if (nrhs<1 || nrhs>2) {
    mexErrMsgIdAndTxt("zlib_compress:nrhs","Invalid number of arguments.");
  }
  if (nlhs!=1) {
    mexErrMsgIdAndTxt("zlib_compress:nlhs","One output required.");
  }

  int level = 6; // default level
  if (nrhs==2) {
    level = (int)mxGetScalar(prhs[1]);
    if (level < 0 || level > 9) {
      mexErrMsgIdAndTxt("zlib_compress:level","Invalid compression level.");
    }
  }

  struct libdeflate_compressor *compressor = libdeflate_alloc_compressor(level);
  if (compressor == NULL) {
    mexErrMsgIdAndTxt("zlib_compress","Internal error. Failed to allocate compressor.");
  }

  void *in = mxGetPr(prhs[0]);
  size_t in_nbytes = mxGetElementSize(prhs[0]) * mxGetNumberOfElements(prhs[0]);
  size_t max_compressed_size = libdeflate_zlib_compress_bound(compressor, in_nbytes);
  plhs[0] = mxCreateUninitNumericArray(1, &max_compressed_size, mxINT8_CLASS, mxREAL);
  void *compressed_data = mxGetPr(plhs[0]);
  size_t compressed_size = libdeflate_zlib_compress(compressor,
                           in, in_nbytes,
                           compressed_data,
                           max_compressed_size);

  if (compressed_size == 0) {
    mexErrMsgIdAndTxt("zlib_compress","Bug in libdeflate_zlib_compress_bound()!.");
  }

  compressed_data = mxRealloc(compressed_data, compressed_size);
  if (compressed_data == NULL) {
    mexErrMsgIdAndTxt("zlib_compress","mxRealloc failed.");
  } else {
    // mxSetInt8s(plhs[0], compressed_data);
    mxSetPr(plhs[0], (double*)compressed_data);
    mxSetM(plhs[0], compressed_size);
  }
}
