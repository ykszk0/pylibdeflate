#include "libdeflate.h"
#include <mex.h>

void mexFunction(int nlhs, mxArray *plhs[],
                  int nrhs, const mxArray *prhs[])
{
  /* check for proper number of arguments */
  if (nrhs != 2) {
    mexErrMsgIdAndTxt("zlib_decompress:nrhs","Invalid number of arguments.");
  }
  if (nlhs!=1) {
    mexErrMsgIdAndTxt("zlib_decompress:nlhs","One output required.");
  }

  size_t decompressed_size = mxGetScalar(prhs[1]); // TODO: mxGetScalar returns double. i.e possible loss of precision

  struct libdeflate_decompressor *decompressor = libdeflate_alloc_decompressor();
  if (decompressor == NULL) {
    mexErrMsgIdAndTxt("zlib_decompress:decompressor","Internal error. Failed to allocate decompressor.");
  }

  void *in = mxGetPr(prhs[0]);
  size_t in_nbytes = mxGetElementSize(prhs[0]) * mxGetNumberOfElements(prhs[0]);
  plhs[0] = mxCreateUninitNumericArray(1, &decompressed_size, mxINT8_CLASS, mxREAL);
  void *decompressed_data = mxGetPr(plhs[0]);
  size_t actual_decompressed_size;
  enum libdeflate_result result = libdeflate_zlib_decompress(decompressor,
                           in, in_nbytes,
                           decompressed_data,
                           decompressed_size, &actual_decompressed_size);

  if (result != LIBDEFLATE_SUCCESS) {
    switch (result) {
    case LIBDEFLATE_INSUFFICIENT_SPACE:
      mexErrMsgIdAndTxt("zlib_decompress:result","Insufficient space.");
      break;
    default:
      mexErrMsgIdAndTxt("zlib_decompress:result","Corrupted file or not in zlib format.");
    }
  }
}
