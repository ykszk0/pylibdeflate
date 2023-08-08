#include <Python.h>
#include <libdeflate.h>
#include <stdio.h>

#define STRINGIFY(n) #n
#define TOSTRING(n) STRINGIFY(n)

PyObject* zlib_compress(PyObject* self, PyObject* args)
{
  Py_buffer buffer;
  int level = 6;
  if (!PyArg_ParseTuple(args, "y*|i", &buffer, &level)) {
    return NULL;
  }
  void *in = buffer.buf;
  size_t in_nbytes = buffer.len;
  struct libdeflate_compressor *compressor = libdeflate_alloc_compressor(level);
  if (compressor == NULL) {
    PyErr_SetString(PyExc_RuntimeError, "compressor allocation failed.");
    return NULL;
  }

  size_t max_compressed_size = libdeflate_zlib_compress_bound(compressor, in_nbytes);
  PyObject* py_bytes = PyBytes_FromStringAndSize(NULL, max_compressed_size);
  if (py_bytes == NULL) {
    libdeflate_free_compressor(compressor);
    PyErr_SetString(PyExc_RuntimeError, "memory allocation failed. " TOSTRING(__LINE__) " at " __FILE__);
    return NULL;
  }
  void *compressed_data = PyBytes_AsString(py_bytes);
  size_t compressed_size = libdeflate_zlib_compress(compressor,
    in,
    in_nbytes,
    compressed_data,
    max_compressed_size);
  libdeflate_free_compressor(compressor);
  if (compressed_size == 0) {
    Py_DECREF(py_bytes);
    PyErr_SetString(PyExc_RuntimeError, "Bug in libdeflate_zlib_compress_bound()!");
    return NULL;
  }
  if (_PyBytes_Resize(&py_bytes, compressed_size) == -1) {
    return NULL;
  }
  return py_bytes;
}

PyObject* zlib_decompress(PyObject* self, PyObject* args)
{
  Py_buffer buffer;
  size_t decompressed_size;
  if (!PyArg_ParseTuple(args, "y*n", &buffer, &decompressed_size)) {
    return NULL;
  }
  void *in = buffer.buf;
  size_t in_nbytes = buffer.len;
  struct libdeflate_decompressor *decompressor = libdeflate_alloc_decompressor();
  if (decompressor == NULL) {
    PyBuffer_Release(&buffer);
    PyErr_SetString(PyExc_RuntimeError, "decompressor allocation failed.");
    return NULL;
  }

  PyObject* py_bytes = PyByteArray_FromStringAndSize(NULL, decompressed_size);
  if (py_bytes == NULL) {
    PyBuffer_Release(&buffer);
    PyErr_SetString(PyExc_RuntimeError, "memory allocation failed. " TOSTRING(__LINE__) " at " __FILE__);
    libdeflate_free_decompressor(decompressor);
    return NULL;
  }
  void *decompressed_data = PyByteArray_AsString(py_bytes);
  size_t actual_decompressed_size;
  enum libdeflate_result result = libdeflate_zlib_decompress(decompressor,
    in,
    in_nbytes,
    decompressed_data,
    decompressed_size, &actual_decompressed_size);
  libdeflate_free_decompressor(decompressor);
  if (result != LIBDEFLATE_SUCCESS) {
    PyBuffer_Release(&buffer);
    Py_DECREF(py_bytes);
    PyErr_SetString(PyExc_RuntimeError, "Corrupted file or not in zlib format");
    return NULL;
  }
  PyBuffer_Release(&buffer);
  return py_bytes;
}

PyObject* gzip_compress(PyObject* self, PyObject* args)
{
  Py_buffer buffer;
  int level = 6;
  if (!PyArg_ParseTuple(args, "y*|i", &buffer, &level)) {
    return NULL;
  }
  void *in = buffer.buf;
  size_t in_nbytes = buffer.len;
  struct libdeflate_compressor *compressor = libdeflate_alloc_compressor(level);
  if (compressor == NULL) {
    PyErr_SetString(PyExc_RuntimeError, "compressor allocation failed.");
    return NULL;
  }

  size_t max_compressed_size = libdeflate_gzip_compress_bound(compressor, in_nbytes);
  PyObject* py_bytes = PyBytes_FromStringAndSize(NULL, max_compressed_size);
  if (py_bytes == NULL) {
    libdeflate_free_compressor(compressor);
    return NULL;
  }
  void *compressed_data = PyBytes_AsString(py_bytes);
  size_t compressed_size = libdeflate_gzip_compress(compressor,
    in,
    in_nbytes,
    compressed_data,
    max_compressed_size);
  libdeflate_free_compressor(compressor);
  if (compressed_size == 0) {
    Py_DECREF(py_bytes);
    PyErr_SetString(PyExc_RuntimeError, "Bug in libdeflate_gzip_compress_bound()!");
    return NULL;
  }
  if (_PyBytes_Resize(&py_bytes, compressed_size) == -1) {
    return NULL;
  }
  return py_bytes;
}

typedef uint32_t u32;
load_u32_gzip(const uint8_t *p)
{
  return ((u32)p[0] << 0) | ((u32)p[1] << 8) |
    ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

PyObject* gzip_decompress(PyObject* self, PyObject* args)
{
  Py_buffer buffer;
  if (!PyArg_ParseTuple(args, "y*n", &buffer)) {
    return NULL;
  }
  void *in = buffer.buf;
  size_t in_nbytes = buffer.len;
  if (in_nbytes < sizeof(u32)) {
    PyBuffer_Release(&buffer);
    PyErr_SetString(PyExc_RuntimeError, "input is not in gzip format.");
    return NULL;
  }

  int decompressed_size = load_u32_gzip((uint8_t *)in + in_nbytes - 4);
  struct libdeflate_decompressor *decompressor = libdeflate_alloc_decompressor();
  if (decompressor == NULL) {
    PyErr_SetString(PyExc_RuntimeError, "decompressor allocation failed.");
    return NULL;
  }

  PyObject* py_bytes = PyByteArray_FromStringAndSize(NULL, decompressed_size);
  if (py_bytes == NULL) {
    PyBuffer_Release(&buffer);
    libdeflate_free_decompressor(decompressor);
    return NULL;
  }
  void *decompressed_data = PyByteArray_AsString(py_bytes);
  size_t actual_decompressed_size;
  enum libdeflate_result result = libdeflate_gzip_decompress(decompressor,
    in,
    in_nbytes,
    decompressed_data,
    decompressed_size, &actual_decompressed_size);
  libdeflate_free_decompressor(decompressor);
  if (result != LIBDEFLATE_SUCCESS) {
    PyBuffer_Release(&buffer);
    Py_DECREF(py_bytes);
    PyErr_SetString(PyExc_RuntimeError, "Corrupted file or not in zlib format");
    return NULL;
  }
  PyBuffer_Release(&buffer);
  return py_bytes;
}

PyObject* read_as_bytearray(PyObject* self, PyObject* args)
{
  const char* filename;
  Py_ssize_t size, offset = 0;
  if (!PyArg_ParseTuple(args, "sn|n", &filename, &size, &offset)) {
    return NULL;
  }

  FILE* fp = fopen(filename, "rb");
  if (fp == NULL) {
    PyErr_SetString(PyExc_RuntimeError, "Can't open file");
    return NULL;
  }
  PyObject* py_bytes = PyByteArray_FromStringAndSize(NULL, size);
  if (py_bytes == NULL) {
    PyErr_SetString(PyExc_RuntimeError, "Failed to allocate array");
    fclose(fp);
    return NULL;
  }

  if (offset > 0) {
    if (fseek(fp, offset, SEEK_SET) != 0) {
      PyErr_SetString(PyExc_RuntimeError, "Something went wrong during fseek");
      fclose(fp);
      Py_DECREF(py_bytes);
      return NULL;
    };
  }
  size_t size_read = fread(PyByteArray_AsString(py_bytes), size, 1, fp);
  if (size_read <= 0) {
    PyErr_SetString(PyExc_RuntimeError, "Something went wrong during fread");
    fclose(fp);
    Py_DECREF(py_bytes);
    return NULL;
  }
  fclose(fp);
  return py_bytes;
}

static PyMethodDef PylibdeflateMethods[] = {
  {"zlib_compress", zlib_compress, METH_VARARGS, "zlib format compression."},
  {"zlib_decompress", zlib_decompress, METH_VARARGS, "zlib format decompression."},
  {"gzip_compress", gzip_compress, METH_VARARGS, "gzip format compression."},
  {"gzip_decompress", gzip_decompress, METH_VARARGS, "gzip format decompression."},
  {"read_as_bytearray", read_as_bytearray, METH_VARARGS, "read binary file as bytearray."},
  {NULL, NULL, 0, NULL} /* Sentinel */
};

static struct PyModuleDef pylibdeflate_module = {
  PyModuleDef_HEAD_INIT,
  "pylibdeflate",   /* name of module */
  NULL, /* module documentation, may be NULL */
  -1,
  PylibdeflateMethods
};

PyMODINIT_FUNC
PyInit_pylibdeflate(void)
{
  return PyModule_Create(&pylibdeflate_module);
}
