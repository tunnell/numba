#include <Python.h>
#include <numpy/ndarraytypes.h>
#include <numpy/arrayscalars.h>
#include "generated_conversions.h"
#include "datetime/_datetime.h"
#include "datetime/np_datetime_strings.h"

/* Utilities copied from Cython/Utility/TypeConversion.c */

/////////////// TypeConversions.proto ///////////////

#if PY_MAJOR_VERSION >= 3
  #define PyInt_FromSize_t             PyLong_FromSize_t
  #define PyInt_AsSsize_t              PyLong_AsSsize_t
#endif

/* Type Conversion Predeclarations */

#define __Numba_PyBytes_FromUString(s) PyBytes_FromString((char*)s)
#define __Numba_PyBytes_AsUString(s)   ((unsigned char*) PyBytes_AsString(s))

#define __Numba_Owned_Py_None(b) (Py_INCREF(Py_None), Py_None)
#define __Numba_PyBool_FromLong(b) ((b) ? (Py_INCREF(Py_True), Py_True) : (Py_INCREF(Py_False), Py_False))
static NUMBA_INLINE int __Numba_PyObject_IsTrue(PyObject*);
static NUMBA_INLINE PyObject* __Numba_PyNumber_Int(PyObject* x);

static NUMBA_INLINE Py_ssize_t __Numba_PyIndex_AsSsize_t(PyObject*);
static NUMBA_INLINE PyObject * __Numba_PyInt_FromSize_t(size_t);
static NUMBA_INLINE size_t __Numba_PyInt_AsSize_t(PyObject*);

#if CYTHON_COMPILING_IN_CPYTHON
#define __Numba_PyFloat_AsDouble(x) (PyFloat_CheckExact(x) ? PyFloat_AS_DOUBLE(x) : PyFloat_AsDouble(x))
#else
#define __Numba_PyFloat_AsDouble(x) PyFloat_AsDouble(x)
#endif
#define __Numba_PyFloat_AsFloat(x) ((float) __Numba_PyFloat_AsDouble(x))

/////////////// TypeConversions ///////////////

/* Type Conversion Functions */

/* Note: __Numba_PyObject_IsTrue is written to minimize branching. */
static NUMBA_INLINE int __Numba_PyObject_IsTrue(PyObject* x) {
   int is_true = x == Py_True;
   if (is_true | (x == Py_False) | (x == Py_None)) return is_true;
   else return PyObject_IsTrue(x);
}

static NUMBA_INLINE PyObject* __Numba_PyNumber_Int(PyObject* x) {
  PyNumberMethods *m;
  const char *name = NULL;
  PyObject *res = NULL;
#if PY_VERSION_HEX < 0x03000000
  if (PyInt_Check(x) || PyLong_Check(x))
#else
  if (PyLong_Check(x))
#endif
    return Py_INCREF(x), x;
  m = Py_TYPE(x)->tp_as_number;
#if PY_VERSION_HEX < 0x03000000
  if (m && m->nb_int) {
    name = "int";
    res = PyNumber_Int(x);
  }
  else if (m && m->nb_long) {
    name = "long";
    res = PyNumber_Long(x);
  }
#else
  if (m && m->nb_int) {
    name = "int";
    res = PyNumber_Long(x);
  }
#endif
  if (res) {
#if PY_VERSION_HEX < 0x03000000
    if (!PyInt_Check(res) && !PyLong_Check(res)) {
#else
    if (!PyLong_Check(res)) {
#endif
      PyErr_Format(PyExc_TypeError,
                   "__%s__ returned non-%s (type %.200s)",
                   name, name, Py_TYPE(res)->tp_name);
      Py_DECREF(res);
      return NULL;
    }
  }
  else if (!PyErr_Occurred()) {
    PyErr_SetString(PyExc_TypeError,
                    "an integer is required");
  }
  return res;
}

static NUMBA_INLINE Py_ssize_t __Numba_PyIndex_AsSsize_t(PyObject* b) {
  Py_ssize_t ival;
  PyObject* x = PyNumber_Index(b);
  if (!x) return -1;
  ival = PyInt_AsSsize_t(x);
  Py_DECREF(x);
  return ival;
}

static NUMBA_INLINE PyObject * __Numba_PyInt_FromSize_t(size_t ival) {
#if PY_VERSION_HEX < 0x02050000
   if (ival <= LONG_MAX)
       return PyInt_FromLong((long)ival);
   else {
       unsigned char *bytes = (unsigned char *) &ival;
       int one = 1; int little = (int)*(unsigned char*)&one;
       return _PyLong_FromByteArray(bytes, sizeof(size_t), little, 0);
   }
#else
   return PyInt_FromSize_t(ival);
#endif
}

static NUMBA_INLINE size_t __Numba_PyInt_AsSize_t(PyObject* x) {
   unsigned PY_LONG_LONG val = __Numba_PyInt_AsUnsignedLongLong(x);
   if (unlikely(val == (unsigned PY_LONG_LONG)-1 && PyErr_Occurred())) {
       return (size_t)-1;
   } else if (unlikely(val != (unsigned PY_LONG_LONG)(size_t)val)) {
       PyErr_SetString(PyExc_OverflowError,
                       "value too large to convert to size_t");
       return (size_t)-1;
   }
   return (size_t)val;
}

/////////////// ObjectAsUCS4.proto ///////////////

static NUMBA_INLINE Py_UCS4 __Numba_PyObject_AsPy_UCS4(PyObject*);

/////////////// ObjectAsUCS4 ///////////////

static NUMBA_INLINE Py_UCS4 __Numba_PyObject_AsPy_UCS4(PyObject* x) {
   long ival;
   if (PyUnicode_Check(x)) {
       Py_ssize_t length;
       #if CYTHON_PEP393_ENABLED
       length = PyUnicode_GET_LENGTH(x);
       if (likely(length == 1)) {
           return PyUnicode_READ_CHAR(x, 0);
       }
       #else
       length = PyUnicode_GET_SIZE(x);
       if (likely(length == 1)) {
           return PyUnicode_AS_UNICODE(x)[0];
       }
       #if Py_UNICODE_SIZE == 2
       else if (PyUnicode_GET_SIZE(x) == 2) {
           Py_UCS4 high_val = PyUnicode_AS_UNICODE(x)[0];
           if (high_val >= 0xD800 && high_val <= 0xDBFF) {
               Py_UCS4 low_val = PyUnicode_AS_UNICODE(x)[1];
               if (low_val >= 0xDC00 && low_val <= 0xDFFF) {
                   return 0x10000 + (((high_val & ((1<<10)-1)) << 10) | (low_val & ((1<<10)-1)));
               }
           }
       }
       #endif
       #endif
       PyErr_Format(PyExc_ValueError,
                    "only single character unicode strings can be converted to Py_UCS4, "
                    "got length %" CYTHON_FORMAT_SSIZE_T "d", length);
       return (Py_UCS4)-1;
   }
   ival = __Numba_PyInt_AsSignedLong(x);
   if (unlikely(ival < 0)) {
       if (!PyErr_Occurred())
           PyErr_SetString(PyExc_OverflowError,
                           "cannot convert negative value to Py_UCS4");
       return (Py_UCS4)-1;
   } else if (unlikely(ival > 1114111)) {
       PyErr_SetString(PyExc_OverflowError,
                       "value too large to convert to Py_UCS4");
       return (Py_UCS4)-1;
   }
   return (Py_UCS4)ival;
}

/////////////// ObjectAsPyUnicode.proto ///////////////

static NUMBA_INLINE Py_UNICODE __Numba_PyObject_AsPy_UNICODE(PyObject*);

/////////////// ObjectAsPyUnicode ///////////////

static NUMBA_INLINE Py_UNICODE __Numba_PyObject_AsPy_UNICODE(PyObject* x) {
    long ival;
    #if CYTHON_PEP393_ENABLED
    #if Py_UNICODE_SIZE > 2
    const long maxval = 1114111;
    #else
    const long maxval = 65535;
    #endif
    #else
    static long maxval = 0;
    #endif
    if (PyUnicode_Check(x)) {
        if (unlikely(__Numba_PyUnicode_GET_LENGTH(x) != 1)) {
            PyErr_Format(PyExc_ValueError,
                         "only single character unicode strings can be converted to Py_UNICODE, "
                         "got length %" CYTHON_FORMAT_SSIZE_T "d", __Numba_PyUnicode_GET_LENGTH(x));
            return (Py_UNICODE)-1;
        }
        #if CYTHON_PEP393_ENABLED
        ival = PyUnicode_READ_CHAR(x, 0);
        #else
        return PyUnicode_AS_UNICODE(x)[0];
        #endif
    } else {
        #if !CYTHON_PEP393_ENABLED
        if (unlikely(!maxval))
            maxval = (long)PyUnicode_GetMax();
        #endif
        ival = __Numba_PyInt_AsSignedLong(x);
    }
    if (unlikely(ival < 0)) {
        if (!PyErr_Occurred())
            PyErr_SetString(PyExc_OverflowError,
                            "cannot convert negative value to Py_UNICODE");
        return (Py_UNICODE)-1;
    } else if (unlikely(ival > maxval)) {
        PyErr_SetString(PyExc_OverflowError,
                        "value too large to convert to Py_UNICODE");
        return (Py_UNICODE)-1;
    }
    return (Py_UNICODE)ival;
}

/* End copy from Cython/Utility/TypeConversion.c */
/* --------------------------------------------- */

#define CUTOFF 0x7fffffffL /* 4-byte platform-independent cutoff */

static PyObject *
__Numba_PyInt_FromLongLong(PY_LONG_LONG value)
{
    assert(sizeof(long) >= 4);
    if (value > CUTOFF || value < -CUTOFF) {
        return PyLong_FromLongLong(value);
    }
    return PyInt_FromLong(value);
}

static PyObject *
__Numba_PyInt_FromUnsignedLongLong(PY_LONG_LONG value)
{
    assert(sizeof(long) >= 4);
    if (value > CUTOFF) {
        return PyLong_FromUnsignedLongLong(value);
    }
    return PyInt_FromLong((long) value);
}

#include "generated_conversions.c"

/* Export all utilities */
static NUMBA_INLINE int __Numba_PyObject_IsTrue(PyObject*);
static NUMBA_INLINE PyObject* __Numba_PyNumber_Int(PyObject* x);

static NUMBA_INLINE Py_ssize_t __Numba_PyIndex_AsSsize_t(PyObject*);
static NUMBA_INLINE PyObject * __Numba_PyInt_FromSize_t(size_t);
static NUMBA_INLINE size_t __Numba_PyInt_AsSize_t(PyObject*);

static npy_datetimestruct iso_datetime2npydatetime(char *datetime_string)
{
    npy_datetimestruct out;
    npy_bool out_local;
    NPY_DATETIMEUNIT out_bestunit;
    npy_bool out_special;

    parse_iso_8601_datetime(datetime_string, strlen(datetime_string), -1, NPY_SAME_KIND_CASTING,
        &out, &out_local, &out_bestunit, &out_special);

    return out;
}

static npy_int64 iso_datetime2timestamp(char *datetime_string)
{
    npy_datetimestruct temp;
    npy_datetime output;
    PyArray_DatetimeMetaData new_meta;

    temp = iso_datetime2npydatetime(datetime_string);
    new_meta.base = lossless_unit_from_datetimestruct(&temp);
    new_meta.num = 1;

    if (convert_datetimestruct_to_datetime(&new_meta, &temp, &output) < 0) {
        return NULL;
    }
    
    return output;
}

static npy_int32 iso_datetime2units(char *datetime_string)
{
    npy_datetimestruct temp;
    temp = iso_datetime2npydatetime(datetime_string);
    return lossless_unit_from_datetimestruct(&temp);
}

static npy_int64 numpydatetime2timestamp(PyObject *numpy_datetime)
{
    return ((PyDatetimeScalarObject*)numpy_datetime)->obval;
}

static npy_int32 numpydatetime2units(PyObject *numpy_datetime)
{
    return ((PyDatetimeScalarObject*)numpy_datetime)->obmeta.base;
}

static npy_int64 numpytimedelta2delta(PyObject *numpy_timedelta)
{
    return ((PyDatetimeScalarObject*)numpy_timedelta)->obval;
}

static npy_int32 numpytimedelta2units(PyObject *numpy_timedelta)
{
    return ((PyDatetimeScalarObject*)numpy_timedelta)->obmeta.base;
}

static PyObject* primitive2pydatetime(
    npy_int64 timestamp,
    npy_int32 units)
{
    /*PyObject *result = PyDateTime_FromDateAndTime(year, month, day,
        hour, min, sec, 0);
    Py_INCREF(result);
    return result;*/
    return NULL;
}

static PyObject* primitive2numpydatetime(
    npy_int64 timestamp,
    npy_int32 units,
    PyDatetimeScalarObject *scalar)
{
    scalar->obval = timestamp;
    scalar->obmeta.base = units;
    scalar->obmeta.num = 1;
 
    return (PyObject*)scalar;
}

static PyObject* primitive2numpytimedelta(
    npy_timedelta timedelta,
    NPY_DATETIMEUNIT units,
    PyDatetimeScalarObject *scalar)
{
    scalar->obval = timedelta;
    scalar->obmeta.base = units;
    scalar->obmeta.num = 1;
 
    return (PyObject*)scalar;
}

static NPY_DATETIMEUNIT get_datetime_casting_unit(
    npy_int32 units1,
    npy_int32 units2)
{
    PyArray_DatetimeMetaData meta1;
    PyArray_DatetimeMetaData meta2;

    memset(&meta1, 0, sizeof(PyArray_DatetimeMetaData));
    meta1.base = units1;
    meta1.num = 1;

    memset(&meta2, 0, sizeof(PyArray_DatetimeMetaData));
    meta2.base = units2;
    meta2.num = 1;

    if (can_cast_datetime64_metadata(&meta1, &meta2, NPY_SAFE_CASTING)) {
        return meta2.base;
    }
    else if (can_cast_datetime64_metadata(&meta2, &meta1, NPY_SAFE_CASTING)) {
        return meta1.base;
    }

    return NPY_FR_GENERIC;
}

static NPY_DATETIMEUNIT get_timedelta_casting_unit(
    npy_int32 units1,
    npy_int32 units2)
{
    PyArray_DatetimeMetaData meta1;
    PyArray_DatetimeMetaData meta2;

    memset(&meta1, 0, sizeof(PyArray_DatetimeMetaData));
    meta1.base = units1;
    meta1.num = 1;

    memset(&meta2, 0, sizeof(PyArray_DatetimeMetaData));
    meta2.base = units2;
    meta2.num = 1;

    if (can_cast_timedelta64_metadata(&meta1, &meta2, NPY_SAFE_CASTING)) {
        return meta2.base;
    }
    else if (can_cast_timedelta64_metadata(&meta2, &meta1, NPY_SAFE_CASTING)) {
        return meta1.base;
    }

    return NPY_FR_GENERIC;
}

static NPY_DATETIMEUNIT get_datetime_timedelta_casting_unit(
    npy_int32 datetime_units,
    npy_int32 timedelta_units)
{
    PyArray_DatetimeMetaData datetime_meta;
    PyArray_DatetimeMetaData timedelta_meta;

    memset(&datetime_meta, 0, sizeof(PyArray_DatetimeMetaData));
    datetime_meta.base = datetime_units;
    datetime_meta.num = 1;

    memset(&timedelta_meta, 0, sizeof(PyArray_DatetimeMetaData));
    timedelta_meta.base = timedelta_units;
    timedelta_meta.num = 1;

    if (can_cast_datetime64_metadata(&datetime_meta,
            &timedelta_meta, NPY_SAFE_CASTING)) {
        return timedelta_meta.base;
    }
    else if (can_cast_datetime64_metadata(&timedelta_meta,
            &datetime_meta, NPY_SAFE_CASTING)) {
        return datetime_meta.base;
    }

    return NPY_FR_GENERIC;
}

static npy_timedelta datetime_subtract(
    npy_int64 timestamp1,
    NPY_DATETIMEUNIT units1,
    npy_int64 timestamp2,
    NPY_DATETIMEUNIT units2,
    NPY_DATETIMEUNIT target_units)
{
    PyArray_DatetimeMetaData src_meta;
    PyArray_DatetimeMetaData dst_meta;
    npy_datetime operand1;
    npy_datetime operand2;

    if (units1 == units2 == target_units) {
        return timestamp1 - timestamp2;
    }

    dst_meta.base = target_units;
    dst_meta.num = 1;

    src_meta.base = units1;
    src_meta.num = 1;
    cast_datetime_to_datetime(&src_meta, &dst_meta, timestamp1, &operand1);

    src_meta.base = units2;
    src_meta.num = 1;
    cast_datetime_to_datetime(&src_meta, &dst_meta, timestamp2, &operand2);

    return operand1 - operand2;
}

static npy_timedelta add_timedelta_to_datetime(
    npy_int64 datetime,
    NPY_DATETIMEUNIT datetime_units,
    npy_int64 timedelta,
    NPY_DATETIMEUNIT timedelta_units,
    NPY_DATETIMEUNIT target_units)
{
    PyArray_DatetimeMetaData src_meta;
    PyArray_DatetimeMetaData dst_meta;
    npy_datetime operand1;
    npy_datetime operand2;

    if (datetime_units == timedelta_units == target_units) {
        return datetime + timedelta;
    }

    dst_meta.base = target_units;
    dst_meta.num = 1;

    src_meta.base = datetime_units;
    src_meta.num = 1;
    cast_datetime_to_datetime(&src_meta, &dst_meta, datetime, &operand1);

    src_meta.base = timedelta_units;
    src_meta.num = 1;
    cast_datetime_to_datetime(&src_meta, &dst_meta, timedelta, &operand2);

    return operand1 + operand2;
}

static npy_datetimestruct extract_datetime(npy_datetime timestamp,
    NPY_DATETIMEUNIT units)
{
    PyArray_DatetimeMetaData meta;
    npy_datetimestruct output;

    meta.base = units;
    meta.num = 1;

    memset(&output, 0, sizeof(npy_datetimestruct));

    convert_datetime_to_datetimestruct(&meta, timestamp, &output);
    return output;
}

static npy_int64 extract_datetime_year(npy_datetime timestamp,
    NPY_DATETIMEUNIT units)
{
    npy_datetimestruct output = extract_datetime(timestamp, units);
    return output.year;
}

static npy_int32 extract_datetime_month(npy_datetime timestamp,
    NPY_DATETIMEUNIT units)
{
    npy_datetimestruct output = extract_datetime(timestamp, units);
    return output.month;
}

static npy_int32 extract_datetime_day(npy_datetime timestamp,
    NPY_DATETIMEUNIT units)
{
    npy_datetimestruct output = extract_datetime(timestamp, units);
    return output.day;
}

static npy_int32 extract_datetime_hour(npy_datetime timestamp,
    NPY_DATETIMEUNIT units)
{
    npy_datetimestruct output = extract_datetime(timestamp, units);
    return output.hour;
}

static npy_int32 extract_datetime_min(npy_datetime timestamp,
    NPY_DATETIMEUNIT units)
{
    npy_datetimestruct output = extract_datetime(timestamp, units);
    return output.min;
}

static npy_int32 extract_datetime_sec(npy_datetime timestamp,
    NPY_DATETIMEUNIT units)
{
    npy_datetimestruct output = extract_datetime(timestamp, units);
    return output.sec;
}

static npy_int32 extract_timedelta_sec(npy_timedelta timedelta,
    NPY_DATETIMEUNIT units)
{
    PyArray_DatetimeMetaData meta1;
    PyArray_DatetimeMetaData meta2;
    npy_timedelta output = 0;

    memset(&meta1, 0, sizeof(PyArray_DatetimeMetaData));
    meta1.base = units;
    meta1.num = 1;

    memset(&meta2, 0, sizeof(PyArray_DatetimeMetaData));
    meta2.base = 7;
    meta2.num = 1;

    cast_timedelta_to_timedelta(&meta1, &meta2, timedelta, &output);
    return output;
}


static npy_int32 convert_timedelta_units_str(char *units_str)
{
    if (units_str == NULL)
        return NPY_FR_GENERIC;

    return parse_datetime_unit_from_string(units_str, strlen(units_str), NULL);
}

static int
export_type_conversion(PyObject *module)
{
    EXPORT_FUNCTION(__Numba_PyInt_AsSignedChar, module, error)
    EXPORT_FUNCTION(__Numba_PyInt_AsUnsignedChar, module, error)
    EXPORT_FUNCTION(__Numba_PyInt_AsSignedShort, module, error)
    EXPORT_FUNCTION(__Numba_PyInt_AsUnsignedShort, module, error)
    EXPORT_FUNCTION(__Numba_PyInt_AsSignedInt, module, error)
    EXPORT_FUNCTION(__Numba_PyInt_AsUnsignedInt, module, error)
    EXPORT_FUNCTION(__Numba_PyInt_AsSignedLong, module, error)
    EXPORT_FUNCTION(__Numba_PyInt_AsUnsignedLong, module, error)
    EXPORT_FUNCTION(__Numba_PyInt_AsSignedLongLong, module, error)
    EXPORT_FUNCTION(__Numba_PyInt_AsUnsignedLongLong, module, error)

    EXPORT_FUNCTION(__Numba_PyIndex_AsSsize_t, module, error);
    EXPORT_FUNCTION(__Numba_PyInt_FromSize_t, module, error);

    EXPORT_FUNCTION(__Numba_PyInt_FromLongLong, module, error);
    EXPORT_FUNCTION(__Numba_PyInt_FromUnsignedLongLong, module, error);

    EXPORT_FUNCTION(iso_datetime2timestamp, module, error);
    EXPORT_FUNCTION(iso_datetime2units, module, error);

    EXPORT_FUNCTION(numpydatetime2timestamp, module, error);
    EXPORT_FUNCTION(numpydatetime2units, module, error);

    EXPORT_FUNCTION(numpytimedelta2delta, module, error);
    EXPORT_FUNCTION(numpytimedelta2units, module, error);

    EXPORT_FUNCTION(primitive2pydatetime, module, error);
    EXPORT_FUNCTION(primitive2numpydatetime, module, error);
    EXPORT_FUNCTION(primitive2numpytimedelta, module, error);

    EXPORT_FUNCTION(get_datetime_casting_unit, module, error);
    EXPORT_FUNCTION(get_timedelta_casting_unit, module, error);
    EXPORT_FUNCTION(get_datetime_timedelta_casting_unit, module, error);
    EXPORT_FUNCTION(extract_datetime_year, module, error);
    EXPORT_FUNCTION(extract_datetime_month, module, error);
    EXPORT_FUNCTION(extract_datetime_day, module, error);
    EXPORT_FUNCTION(extract_datetime_hour, module, error);
    EXPORT_FUNCTION(extract_datetime_min, module, error);
    EXPORT_FUNCTION(extract_datetime_sec, module, error);
    EXPORT_FUNCTION(datetime_subtract, module, error);
    EXPORT_FUNCTION(add_timedelta_to_datetime, module, error);
    EXPORT_FUNCTION(extract_timedelta_sec, module, error);
    EXPORT_FUNCTION(convert_timedelta_units_str, module, error);

    return 0;
error:
    return -1;
}
