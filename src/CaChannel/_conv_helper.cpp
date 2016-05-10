
#define FormatPlaintoValue(COUNT, VAL, PYTYPE, FORMAT) \
    {\
        register unsigned long cur;\
        if(COUNT==1)\
            value = FORMAT(*(PYTYPE*)VAL);\
        else{\
            register PYTYPE *vp = (PYTYPE *) VAL;\
            value = PyList_New(count);\
            for (cur=0;cur < count; cur++){\
                PyObject *ent=FORMAT(*(vp++));\
                PyList_SetItem(value,cur,ent);\
            }\
        }\
    }
#define FormatDBRtoValue(COUNT, DBRTYPE, VP, PYTYPE, FORMAT) \
    {\
        register unsigned long cur;\
        if(COUNT==1)\
            value = FORMAT((PYTYPE) cval->value);\
        else{\
            value = PyList_New(count);\
            register dbr_##DBRTYPE##_t *vp=(dbr_##DBRTYPE##_t *)(VP);\
            for (cur=0;cur < count; cur++){\
                PyObject *ent=FORMAT((PYTYPE) *(vp++));\
                PyList_SetItem(value,cur,ent);\
            }\
        }\
    }

#define FormatPlaintoValueArray(COUNT, VAL, PYTYPE, FORMAT, NPTYPE) \
    {\
        if(COUNT==1)\
            value = FORMAT(*((PYTYPE*)VAL));\
        else{\
            npy_intp ndims[]  = {COUNT};\
            value =  PyArray_SimpleNew(1, ndims, NPTYPE); \
            memcpy(PyArray_DATA(value), VAL, COUNT*sizeof(PYTYPE)); \
        }\
    }
#define FormatDBRtoValueArray(COUNT, VP, PYTYPE, FORMAT,  NPTYPE) \
    {\
        if(COUNT==1)\
            value = FORMAT((PYTYPE) cval->value);\
        else{\
            npy_intp ndims[]  = {COUNT};\
            value =  PyArray_SimpleNew(1, ndims, NPTYPE); \
            memcpy(PyArray_DATA(value), VP, COUNT*sizeof(PYTYPE)); \
        }\
    }

#define FormatDBRStringtoValue(COUNT, DBRTYPE, VP, PYTYPE, FORMAT) \
    {\
        register unsigned long cur;\
        if(COUNT==1)\
            value = FORMAT(*(PYTYPE) cval->value);\
        else{\
            value = PyList_New(count);\
            register dbr_##DBRTYPE##_t *vp=(dbr_##DBRTYPE##_t *)(VP);\
            for (cur=0;cur < count; cur++){\
                PyObject *ent=FORMAT(*(PYTYPE) *(vp++));\
                PyList_SetItem(value,cur,ent);\
            }\
        }\
    }


PyObject * 
_convert_ca_to_Python(chtype type, 
                    unsigned long count, 
                    void *val, 
                    int status, 
                    int use_numpy){
    PyObject *arglist = NULL;
    PyObject *value   = NULL;

    if(!val){
        if(CaError){
            PyErr_SetString(CaError, "Null pointer as return value.");  
        }
    return (PyObject *) NULL;
    }
  
    /* build arglist, value is inserted */
    switch(type){
    case DBR_STRING:
        FormatPlaintoValue(count, val, dbr_string_t, PyString_FromString);
        arglist = Py_BuildValue("((Oii))", value, -1, status);
        break;
    case DBR_SHORT: 
        if(use_numpy == 0)
            FormatPlaintoValue(count, val, dbr_short_t,  PyInt_FromLong)
        #ifdef WITH_NUMPY
        else
            FormatPlaintoValueArray(count, val, dbr_short_t,  PyInt_FromLong,  NPY_SHORT)
        #endif
        arglist = Py_BuildValue("((Oii))", value, -1, status);
        break;
    case DBR_FLOAT:
        if(use_numpy == 0)
            FormatPlaintoValue(count, val, dbr_float_t,  PyFloat_FromDouble)
        #ifdef WITH_NUMPY
        else
            FormatPlaintoValueArray(count, val, dbr_float_t,  PyFloat_FromDouble,  NPY_FLOAT)
        #endif
        arglist = Py_BuildValue("((Oii))", value, -1, status);
        break;
    case DBR_ENUM:
        if(use_numpy == 0)
            FormatPlaintoValue(count, val, dbr_enum_t,   PyInt_FromLong)
        #ifdef WITH_NUMPY
        else
            FormatPlaintoValueArray(count, val, dbr_enum_t,  PyInt_FromLong,  NPY_USHORT)
        #endif
        arglist = Py_BuildValue("((Oii))", value, -1, status);
        break;
    case DBR_CHAR:
        if (use_numpy == 0)
            FormatPlaintoValue(count, val, dbr_char_t,   PyInt_FromLong)
        #ifdef WITH_NUMPY
        else
            FormatPlaintoValueArray(count, val, dbr_char_t,   PyInt_FromLong,  NPY_BYTE)
        #endif
        arglist = Py_BuildValue("((Oii))", value, -1, status);
        break;
    case DBR_LONG:
        if (use_numpy == 0)
            FormatPlaintoValue(count, val, dbr_long_t,   PyInt_FromLong)
        #ifdef WITH_NUMPY
        else
            FormatPlaintoValueArray(count, val, dbr_long_t,   PyInt_FromLong,  NPY_INT)
        #endif
        arglist = Py_BuildValue("((Oii))", value, -1, status);
        break;
    case DBR_DOUBLE:
        if (use_numpy == 0)
            FormatPlaintoValue(count, val, dbr_double_t, PyFloat_FromDouble)
        #ifdef WITH_NUMPY
        else
            FormatPlaintoValueArray(count, val, dbr_double_t, PyFloat_FromDouble, NPY_DOUBLE)
        #endif
        arglist = Py_BuildValue("((Oii))", value, -1, status);
        break;

    case DBR_STS_STRING:
    case DBR_GR_STRING:
    case DBR_CTRL_STRING:
    {
        struct dbr_sts_string  *cval=(struct dbr_sts_string  *)val;
        FormatDBRStringtoValue(count, string,  cval->value , dbr_string_t_ptr, PyString_FromString); 
        arglist=Py_BuildValue("((Oii))",value,
                 (int) cval->severity,(int) cval->status);
    }
        break;
    case DBR_STS_SHORT: 
    {
        struct dbr_sts_short  *cval=(struct dbr_sts_short  *)val;
        if (use_numpy == 0)
            FormatDBRtoValue(count, short, &(cval->value), dbr_short_t,   PyInt_FromLong)
        #ifdef WITH_NUMPY
        else
            FormatDBRtoValueArray(count, &(cval->value), dbr_short_t,   PyInt_FromLong, NPY_SHORT)
        #endif
        arglist=Py_BuildValue("((Oii))",value,
                 (int) cval->severity,(int) cval->status);
    }
        break;
    case DBR_STS_FLOAT:
    {
        struct dbr_sts_float  *cval=(struct dbr_sts_float  *)val;
        if (use_numpy == 0)
            FormatDBRtoValue(count, float, &(cval->value), dbr_float_t, PyFloat_FromDouble)
        #ifdef WITH_NUMPY
        else
            FormatDBRtoValueArray(count, &(cval->value), dbr_float_t, PyFloat_FromDouble, NPY_FLOAT)
        #endif
        arglist =Py_BuildValue("((Oii))",value,
                 (int) cval->severity,(int) cval->status);
    }
        break;
    case DBR_STS_ENUM:
    {
        struct dbr_sts_enum  *cval=(struct dbr_sts_enum  *)val;    
        if (use_numpy == 0)
            FormatDBRtoValue(count, enum, &(cval->value),  dbr_enum_t,     PyInt_FromLong)
        #ifdef WITH_NUMPY
        else
            FormatDBRtoValueArray(count,  &(cval->value), dbr_enum_t,      PyInt_FromLong, NPY_USHORT)
        #endif
        arglist=Py_BuildValue("((Oii))",value,
                 (int) cval->severity,(int) cval->status);
    }
        break;
    case DBR_STS_CHAR:
    {
        struct dbr_sts_char  *cval=(struct dbr_sts_char  *)val;    
        if (use_numpy == 0)
            FormatDBRtoValue(count, char, &(cval->value), dbr_char_t,     PyInt_FromLong)
        #ifdef WITH_NUMPY
        else
            FormatDBRtoValueArray(count,  &(cval->value), dbr_char_t,     PyInt_FromLong, NPY_BYTE)
        #endif
        arglist=Py_BuildValue("((Oii))",value,
                 (int) cval->severity,(int) cval->status);
    }
        break;
    case DBR_STS_LONG:
    {
        struct dbr_sts_long  *cval=(struct dbr_sts_long  *)val;
        if (use_numpy == 0)
            FormatDBRtoValue(count, long, &(cval->value), dbr_long_t,     PyInt_FromLong)
        #ifdef WITH_NUMPY
        else
            FormatDBRtoValueArray(count,  &(cval->value), dbr_long_t,     PyInt_FromLong, NPY_INT)
        #endif
        arglist=Py_BuildValue("((Oii))",value,
                 (int) cval->severity,(int) cval->status);
    }
        break;
    case DBR_STS_DOUBLE:
    {   
        struct dbr_sts_double  *cval=(struct dbr_sts_double  *)val;
        if (use_numpy == 0)
            FormatDBRtoValue(count, double, &(cval->value), dbr_double_t, PyFloat_FromDouble)
        #ifdef WITH_NUMPY
        else
            FormatDBRtoValueArray(count,    &(cval->value), dbr_double_t, PyFloat_FromDouble, NPY_DOUBLE)
        #endif
        arglist=Py_BuildValue("((Oii))",value,
                 (int) cval->severity,(int) cval->status);
    }
        break;

    case DBR_TIME_STRING:
    {
        struct dbr_time_string  *cval=(struct dbr_time_string  *)val;
        FormatDBRStringtoValue(count, string,  cval->value, dbr_string_t_ptr, PyString_FromString);
        arglist=Py_BuildValue("((Oiid))",value,
                 (int) cval->severity,(int) cval->status, TS2secs(cval->stamp));
    }
        break;
    case DBR_TIME_SHORT: 
    {
        struct dbr_time_short  *cval=(struct dbr_time_short  *)val;
        if (use_numpy == 0)
            FormatDBRtoValue(count, short, &(cval->value), dbr_short_t,      PyInt_FromLong)
        #ifdef WITH_NUMPY
        else
            FormatDBRtoValueArray(count,   &(cval->value), dbr_short_t,      PyInt_FromLong, NPY_SHORT)
        #endif
        arglist=Py_BuildValue("((Oiid))",value,
                 (int) cval->severity,(int) cval->status, TS2secs(cval->stamp));
    }
        break;
    case DBR_TIME_FLOAT:
    {
        struct dbr_time_float  *cval=(struct dbr_time_float  *)val;
        if (use_numpy == 0)
            FormatDBRtoValue(count, float, &(cval->value), dbr_float_t,      PyFloat_FromDouble)
        #ifdef WITH_NUMPY
        else
            FormatDBRtoValueArray(count,   &(cval->value), dbr_float_t,      PyFloat_FromDouble, NPY_FLOAT)
        #endif
        arglist=Py_BuildValue("((Oiid))",value,
                 (int) cval->severity,(int) cval->status, TS2secs(cval->stamp));
    }
        break;
    case DBR_TIME_ENUM:
    {
        struct dbr_time_enum  *cval=(struct dbr_time_enum  *)val;    
        if (use_numpy == 0)
            FormatDBRtoValue(count, enum, &(cval->value), dbr_enum_t,        PyInt_FromLong)
        #ifdef WITH_NUMPY
        else
            FormatDBRtoValueArray(count,  &(cval->value), dbr_enum_t,        PyInt_FromLong, NPY_USHORT)
        #endif
        arglist=Py_BuildValue("((Oiid))",value,
                 (int) cval->severity,(int) cval->status, TS2secs(cval->stamp));
    }
        break;
    case DBR_TIME_CHAR:
    {
        struct dbr_time_char  *cval=(struct dbr_time_char  *)val;    
        if (use_numpy == 0)
            FormatDBRtoValue(count, char, &(cval->value), dbr_char_t,        PyInt_FromLong)
        #ifdef WITH_NUMPY
        else
            FormatDBRtoValueArray(count,  &(cval->value), dbr_char_t,        PyInt_FromLong, NPY_BYTE)
        #endif
        arglist=Py_BuildValue("((Oiid))",value,
                 (int) cval->severity,(int) cval->status, TS2secs(cval->stamp));
    }
        break;
    case DBR_TIME_LONG:
    {
        struct dbr_time_long  *cval=(struct dbr_time_long  *)val;
        if (use_numpy == 0)
            FormatDBRtoValue(count, long, &(cval->value), dbr_long_t,        PyInt_FromLong)
        #ifdef WITH_NUMPY
        else
            FormatDBRtoValueArray(count,  &(cval->value), dbr_long_t,        PyInt_FromLong, NPY_INT)
        #endif
        arglist=Py_BuildValue("((Oiid))",value,
                 (int) cval->severity,(int) cval->status, TS2secs(cval->stamp));
    }
        break;
    case DBR_TIME_DOUBLE:        
    {   
        struct dbr_time_double  *cval=(struct dbr_time_double  *)val;
        if (use_numpy == 0)
            FormatDBRtoValue(count, double, &(cval->value), dbr_double_t,    PyFloat_FromDouble)
        #ifdef WITH_NUMPY
        else
            FormatDBRtoValueArray(count,    &(cval->value), dbr_double_t,    PyFloat_FromDouble, NPY_DOUBLE)
        #endif
        arglist=Py_BuildValue("((Oiid))",value,
                 (int) cval->severity,(int) cval->status, TS2secs(cval->stamp));
    }
        break;

    case DBR_GR_SHORT:
    {
        struct dbr_gr_short  *cval=(struct dbr_gr_short  *)val;
        if (use_numpy == 0)
            FormatDBRtoValue(count, short,  &(cval->value), dbr_short_t,     PyInt_FromLong)
        #ifdef WITH_NUMPY
        else
             FormatDBRtoValueArray(count,   &(cval->value), dbr_short_t,     PyInt_FromLong, NPY_SHORT)
        #endif
        arglist=Py_BuildValue("((Oiid(siiiiii)))",
                value,
                cval->severity,
                cval->status,
                0.0,
                cval->units,
                cval->upper_disp_limit,
                cval->lower_disp_limit,
                cval->upper_alarm_limit,
                cval->upper_warning_limit,
                cval->lower_alarm_limit,
                cval->lower_warning_limit
                );
    }
        break;
    case DBR_GR_FLOAT:
    {
        struct dbr_gr_float  *cval=(struct dbr_gr_float  *)val;        
        if (use_numpy == 0)
            FormatDBRtoValue(count, float,  &(cval->value), dbr_float_t,     PyFloat_FromDouble)
        #ifdef WITH_NUMPY
        else
            FormatDBRtoValueArray(count,    &(cval->value), dbr_float_t,     PyFloat_FromDouble, NPY_FLOAT)
        #endif
        arglist=Py_BuildValue("((Oiid(sffffffi)))",
                value,
                cval->severity,
                cval->status,
                0.0,
                cval->units,
                cval->upper_disp_limit,
                cval->lower_disp_limit,
                cval->upper_alarm_limit,
                cval->upper_warning_limit,
                cval->lower_alarm_limit,
                cval->lower_warning_limit,
                cval->precision
                ); 
    }
        break;
    case DBR_GR_ENUM:
    {
        struct dbr_gr_enum  *cval=(struct dbr_gr_enum  *)val;
        if (use_numpy == 0)
            FormatDBRtoValue(count, enum,  &(cval->value), dbr_enum_t,       PyInt_FromLong)
        #ifdef WITH_NUMPY
        else
            FormatDBRtoValueArray(count,   &(cval->value), dbr_enum_t,       PyInt_FromLong, NPY_USHORT)
        #endif
        char (*strs)[][MAX_ENUM_STRING_SIZE]=(char (*)[][MAX_ENUM_STRING_SIZE]) &cval->strs;
        unsigned long nstr=cval->no_str,i;
        PyObject *ptup=PyTuple_New(nstr);
        for(i=0; i< nstr;i++){
            PyTuple_SET_ITEM(ptup,i,PyString_FromString((*strs)[i]));
        }
        arglist=Py_BuildValue("((Oiid(iO)))",
            value,
            cval->severity,
            cval->status,
            0.0,
            cval->no_str,
            ptup
            );
        Py_XDECREF(ptup);
    }
        break; 
    case DBR_GR_CHAR:
    {
        struct dbr_gr_char  *cval=(struct dbr_gr_char  *)val;        
        if (use_numpy == 0)
            FormatDBRtoValue(count, char,  &(cval->value), dbr_char_t,        PyInt_FromLong)
        #ifdef WITH_NUMPY
        else
            FormatDBRtoValueArray(count,   &(cval->value), dbr_char_t,        PyInt_FromLong, NPY_BYTE)
        #endif
        arglist=Py_BuildValue("((Oiid(sbbbbbb)))",
                value,
                cval->severity,
                cval->status,
                0.0,
                cval->units,
                cval->upper_disp_limit,
                cval->lower_disp_limit,
                cval->upper_alarm_limit,
                cval->upper_warning_limit,
                cval->lower_alarm_limit,
                cval->lower_warning_limit
                ); 
    }
        break;
    case DBR_GR_LONG:
    {
        struct dbr_gr_long  *cval=(struct dbr_gr_long  *)val;        
        if (use_numpy == 0)
            FormatDBRtoValue(count, long,  &(cval->value), dbr_long_t,       PyInt_FromLong)
        #ifdef WITH_NUMPY
        else
            FormatDBRtoValueArray(count,   &(cval->value), dbr_long_t,       PyInt_FromLong, NPY_INT) 
        #endif
        arglist=Py_BuildValue("((Oiid(siiiiii)))",
                value,
                cval->severity,
                cval->status,
                0.0,
                cval->units,
                cval->upper_disp_limit,
                cval->lower_disp_limit,
                cval->upper_alarm_limit,
                cval->upper_warning_limit,
                cval->lower_alarm_limit,
                cval->lower_warning_limit
                );
    }
        break;

    case DBR_GR_DOUBLE:
    {
        struct dbr_gr_double  *cval=(struct dbr_gr_double  *)val;        
        if (use_numpy)
            FormatDBRtoValue(count, double,  &(cval->value), dbr_double_t,    PyFloat_FromDouble)
        #ifdef WITH_NUMPY
        else
            FormatDBRtoValueArray(count,     &(cval->value), dbr_double_t,    PyFloat_FromDouble, NPY_DOUBLE)
        #endif
        arglist=Py_BuildValue("((Oiid(sddddddi)))",
                value,
                cval->severity,
                cval->status,
                0.0,
                cval->units,
                cval->upper_disp_limit,
                cval->lower_disp_limit,
                cval->upper_alarm_limit,
                cval->upper_warning_limit,
                cval->lower_alarm_limit,
                cval->lower_warning_limit,
                cval->precision
                );
    }
        break;

    case DBR_CTRL_SHORT:
    {
        struct dbr_ctrl_short  *cval=(struct dbr_ctrl_short  *)val;
        if (use_numpy == 0)
            FormatDBRtoValue(count, short,  &(cval->value), dbr_short_t,      PyInt_FromLong)
        #ifdef WITH_NUMPY
        else
            FormatDBRtoValueArray(count,    &(cval->value), dbr_short_t,      PyInt_FromLong, NPY_SHORT)
        #endif
        arglist=Py_BuildValue("((Oiid(siiiiiiii)))",
                value,
                cval->severity,
                cval->status,
                0.0,
                cval->units,
                cval->upper_disp_limit,
                cval->lower_disp_limit,
                cval->upper_alarm_limit,
                cval->upper_warning_limit,
                cval->lower_alarm_limit,
                cval->lower_warning_limit,
                cval->upper_ctrl_limit,
                cval->lower_ctrl_limit
                );
    }
        break;
    case DBR_CTRL_FLOAT:
    {
        struct dbr_ctrl_float  *cval=(struct dbr_ctrl_float  *)val;        
        if (use_numpy == 0)
            FormatDBRtoValue(count, float,  &(cval->value), dbr_float_t,      PyFloat_FromDouble)
        #ifdef WITH_NUMPY
        else
             FormatDBRtoValueArray(count,   &(cval->value), dbr_float_t,      PyFloat_FromDouble, NPY_FLOAT) 
        #endif
        arglist=Py_BuildValue("((Oiid(sffffffffi)))",
                value,
                cval->severity,
                cval->status,
                0.0,
                cval->units,
                cval->upper_disp_limit,
                cval->lower_disp_limit,
                cval->upper_alarm_limit,
                cval->upper_warning_limit,
                cval->lower_alarm_limit,
                cval->lower_warning_limit,
                cval->upper_ctrl_limit,
                cval->lower_ctrl_limit,
                cval->precision
                ); 
    }
        break;
    case DBR_CTRL_ENUM:
    {
        struct dbr_ctrl_enum  *cval=(struct dbr_ctrl_enum  *)val;
        if (use_numpy == 0)
            FormatDBRtoValue(count, enum,  &(cval->value), dbr_enum_t,        PyInt_FromLong)
        #ifdef WITH_NUMPY
        else
            FormatDBRtoValueArray(count,   &(cval->value), dbr_enum_t,        PyInt_FromLong, NPY_USHORT)
        #endif
        char (*strs)[][MAX_ENUM_STRING_SIZE]=(char (*)[][MAX_ENUM_STRING_SIZE]) &cval->strs;
        unsigned long nstr=cval->no_str,i;
        PyObject *ptup=PyTuple_New(nstr);
        for(i=0; i< nstr;i++){
            PyTuple_SET_ITEM(ptup,i,PyString_FromString((*strs)[i]));
        }
        arglist=Py_BuildValue("((Oiid(iO)))",
            value,
            cval->severity,
            cval->status,
            0.0,
            cval->no_str,
            ptup
            );
        Py_XDECREF(ptup);
    }
        break; 
    case DBR_CTRL_CHAR:
    {
        struct dbr_ctrl_char  *cval=(struct dbr_ctrl_char  *)val;        
        if (use_numpy == 0)
            FormatDBRtoValue(count, char,  &(cval->value), dbr_char_t,        PyInt_FromLong)
        #ifdef WITH_NUMPY
        else
            FormatDBRtoValueArray(count,   &(cval->value), dbr_char_t,        PyInt_FromLong, NPY_BYTE)
        #endif
        arglist=Py_BuildValue("((Oiid(sbbbbbbbb)))",
                value,
                cval->severity,
                cval->status,
                0.0,
                cval->units,
                cval->upper_disp_limit,
                cval->lower_disp_limit,
                cval->upper_alarm_limit,
                cval->upper_warning_limit,
                cval->lower_alarm_limit,
                cval->lower_warning_limit,
                cval->upper_ctrl_limit,
                cval->lower_ctrl_limit
                ); 
    }
        break;
    case DBR_CTRL_LONG:
    {
        struct dbr_ctrl_long  *cval=(struct dbr_ctrl_long  *)val;        
        if (use_numpy == 0)
            FormatDBRtoValue(count, long,  &(cval->value), dbr_long_t,        PyInt_FromLong)
        #ifdef WITH_NUMPY
        else
            FormatDBRtoValueArray(count,   &(cval->value), dbr_long_t,        PyInt_FromLong, NPY_INT)
        #endif
        arglist=Py_BuildValue("((Oiid(siiiiiiii)))",
                value,
                cval->severity,
                cval->status,
                0.0,
                cval->units,
                cval->upper_disp_limit,
                cval->lower_disp_limit,
                cval->upper_alarm_limit,
                cval->upper_warning_limit,
                cval->lower_alarm_limit,
                cval->lower_warning_limit,
                cval->upper_ctrl_limit,
                cval->lower_ctrl_limit
                );
    }
        break;

    case DBR_CTRL_DOUBLE:
    {
        struct dbr_ctrl_double  *cval=(struct dbr_ctrl_double  *)val;        
        if (use_numpy == 0)
            FormatDBRtoValue(count, double,  &(cval->value), dbr_double_t,    PyFloat_FromDouble)
        #ifdef WITH_NUMPY
        else
            FormatDBRtoValueArray(count,     &(cval->value), dbr_double_t,    PyFloat_FromDouble, NPY_DOUBLE)
        #endif
        arglist=Py_BuildValue("((Oiid(sddddddddi)))",
                value,
                cval->severity,
                cval->status,
                0.0,
                cval->units,
                cval->upper_disp_limit,
                cval->lower_disp_limit,
                cval->upper_alarm_limit,
                cval->upper_warning_limit,
                cval->lower_alarm_limit,
                cval->lower_warning_limit,
                cval->upper_ctrl_limit,
                cval->lower_ctrl_limit,
                cval->precision
                );
    }
        break;
    }
    Py_XDECREF(value);
    return arglist;
}
