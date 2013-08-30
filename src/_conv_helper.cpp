#define FormatPlaintoValue(COUNT, VAL, PYTYPE, FORMAT) \
    {\
        register unsigned long cur;\
        if(COUNT==1)\
            value = Py_BuildValue(FORMAT,*((PYTYPE*)VAL));\
        else{\
            register PYTYPE *vp = (PYTYPE *) VAL;\
            value = PyList_New(0);\
            for (cur=0;cur < count; cur++){\
                PyObject *ent=Py_BuildValue(FORMAT, *vp++);\
                PyList_Append(value,ent);\
                Py_XDECREF(ent);\
            }\
        }\
    }
#define FormatDBRtoValue(COUNT, DBRTYPE, VP, PYTYPE, FORMAT) \
    {\
        register unsigned long cur;\
        if(COUNT==1)\
            value = Py_BuildValue(FORMAT,(PYTYPE) cval->value);\
        else{\
            value = PyList_New(0);\
            register dbr_##DBRTYPE##_t *vp=(dbr_##DBRTYPE##_t *)(VP);\
            for (cur=0;cur < count; cur++){\
                PyObject *ent=Py_BuildValue(FORMAT,(PYTYPE) *vp++);\
                PyList_Append(value,ent);\
                Py_XDECREF(ent);\
            }\
        }\
    }


PyObject * 
_convert_ca_to_Python(chtype type, 
                    unsigned long count, 
                    void *val, 
                    int status){
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
        FormatPlaintoValue(count, val, dbr_string_t, "z");
        arglist = Py_BuildValue("((Oii))", value, -1, status);
        break;
    case DBR_SHORT: 
        FormatPlaintoValue(count, val, dbr_short_t,  "i");
        arglist = Py_BuildValue("((Oii))", value, -1, status);
        break;
    case DBR_FLOAT:
        FormatPlaintoValue(count, val, dbr_float_t,  "f");
        arglist = Py_BuildValue("((Oii))", value, -1, status);
        break;
    case DBR_ENUM:
        FormatPlaintoValue(count, val, dbr_enum_t,   "i");
        arglist = Py_BuildValue("((Oii))", value, -1, status);
        break;
    case DBR_CHAR:
        FormatPlaintoValue(count, val, dbr_char_t,   "b");
        arglist = Py_BuildValue("((Oii))", value, -1, status);
        break;
    case DBR_LONG:
        FormatPlaintoValue(count, val, dbr_long_t,   "i");
        arglist = Py_BuildValue("((Oii))", value, -1, status);
        break;
    case DBR_DOUBLE: 
        FormatPlaintoValue(count, val, dbr_double_t, "d");
        arglist = Py_BuildValue("((Oii))", value, -1, status);
        break;

    case DBR_STS_STRING:
    case DBR_GR_STRING:
    case DBR_CTRL_STRING:
    {
        struct dbr_sts_string  *cval=(struct dbr_sts_string  *)val;
        FormatDBRtoValue(count, string,  cval->value , dbr_string_t_ptr, "z");
        arglist=Py_BuildValue("((Oii))",value,
                 (int) cval->severity,(int) cval->status);
    }
        break;
    case DBR_STS_SHORT: 
    {
        struct dbr_sts_short  *cval=(struct dbr_sts_short  *)val;
        FormatDBRtoValue(count, short, &(cval->value), dbr_short_t,   "i");
        arglist=Py_BuildValue("((Oii))",value,
                 (int) cval->severity,(int) cval->status);
    }
        break;
    case DBR_STS_FLOAT:
    {
        struct dbr_sts_float  *cval=(struct dbr_sts_float  *)val;
        FormatDBRtoValue(count, float, &(cval->value), dbr_float_t,   "f");
        arglist=Py_BuildValue("((Oii))",value,
                 (int) cval->severity,(int) cval->status);
    }
        break;
    case DBR_STS_ENUM:
    {
        struct dbr_sts_enum  *cval=(struct dbr_sts_enum  *)val;    
        FormatDBRtoValue(count, enum, &(cval->value),  dbr_enum_t,    "i");
        arglist=Py_BuildValue("((Oii))",value,
                 (int) cval->severity,(int) cval->status);
    }
        break;
    case DBR_STS_CHAR:
    {
        struct dbr_sts_char  *cval=(struct dbr_sts_char  *)val;    
        FormatDBRtoValue(count, char, &(cval->value), dbr_char_t,     "b");
        arglist=Py_BuildValue("((Oii))",value,
                 (int) cval->severity,(int) cval->status);
    }
        break;
    case DBR_STS_LONG:
    {
        struct dbr_sts_long  *cval=(struct dbr_sts_long  *)val;
        FormatDBRtoValue(count, long, &(cval->value), dbr_long_t,     "i");
        arglist=Py_BuildValue("((Oii))",value,
                 (int) cval->severity,(int) cval->status);
    }
        break;
    case DBR_STS_DOUBLE:
    {   
        struct dbr_sts_double  *cval=(struct dbr_sts_double  *)val;
        FormatDBRtoValue(count, double, &(cval->value), dbr_double_t, "d");
        arglist=Py_BuildValue("((Oii))",value,
                 (int) cval->severity,(int) cval->status);
    }
        break;

    case DBR_TIME_STRING:
    {
        struct dbr_time_string  *cval=(struct dbr_time_string  *)val;
        FormatDBRtoValue(count, string,  cval->value , dbr_string_t_ptr, "z");
        arglist=Py_BuildValue("((Oiid))",value,
                 (int) cval->severity,(int) cval->status, TS2secs(cval->stamp));
    }
        break;
    case DBR_TIME_SHORT: 
    {
        struct dbr_time_short  *cval=(struct dbr_time_short  *)val;
        FormatDBRtoValue(count, short, &(cval->value), dbr_short_t,      "i");
        arglist=Py_BuildValue("((Oiid))",value,
                 (int) cval->severity,(int) cval->status, TS2secs(cval->stamp));
    }
        break;
    case DBR_TIME_FLOAT:
    {
        struct dbr_time_float  *cval=(struct dbr_time_float  *)val;
        FormatDBRtoValue(count, float, &(cval->value), dbr_float_t,      "f");
        arglist=Py_BuildValue("((Oiid))",value,
                 (int) cval->severity,(int) cval->status, TS2secs(cval->stamp));
    }
        break;
    case DBR_TIME_ENUM:
    {
        struct dbr_time_enum  *cval=(struct dbr_time_enum  *)val;    
        FormatDBRtoValue(count, enum, &(cval->value), dbr_enum_t,        "i");
        arglist=Py_BuildValue("((Oiid))",value,
                 (int) cval->severity,(int) cval->status, TS2secs(cval->stamp));
    }
        break;
    case DBR_TIME_CHAR:
    {
        struct dbr_time_char  *cval=(struct dbr_time_char  *)val;    
        FormatDBRtoValue(count, char, &(cval->value), dbr_char_t,        "b");
        arglist=Py_BuildValue("((Oiid))",value,
                 (int) cval->severity,(int) cval->status, TS2secs(cval->stamp));
    }
        break;
    case DBR_TIME_LONG:
    {
        struct dbr_time_long  *cval=(struct dbr_time_long  *)val;
        FormatDBRtoValue(count, long, &(cval->value), dbr_long_t,        "i");
        arglist=Py_BuildValue("((Oiid))",value,
                 (int) cval->severity,(int) cval->status, TS2secs(cval->stamp));
    }
        break;
    case DBR_TIME_DOUBLE:        
    {   
        struct dbr_time_double  *cval=(struct dbr_time_double  *)val;
        FormatDBRtoValue(count, double, &(cval->value), dbr_double_t,    "d");
        arglist=Py_BuildValue("((Oiid))",value,
                 (int) cval->severity,(int) cval->status, TS2secs(cval->stamp));
    }
        break;

    case DBR_GR_SHORT:
    {
        struct dbr_gr_short  *cval=(struct dbr_gr_short  *)val;
        FormatDBRtoValue(count, short,  &(cval->value), dbr_short_t,     "i");
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
        FormatDBRtoValue(count, float,  &(cval->value), dbr_float_t,     "f");
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
        FormatDBRtoValue(count, enum,  &(cval->value), dbr_enum_t,       "i"); 
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
        FormatDBRtoValue(count, char,  &(cval->value), dbr_char_t,        "b");
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
        FormatDBRtoValue(count, long,  &(cval->value), dbr_long_t,       "i");
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
        FormatDBRtoValue(count, double,  &(cval->value), dbr_double_t,    "d");
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
        FormatDBRtoValue(count, short,  &(cval->value), dbr_short_t,      "i");
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
        FormatDBRtoValue(count, float,  &(cval->value), dbr_float_t,      "f");
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
        FormatDBRtoValue(count, enum,  &(cval->value), dbr_enum_t,        "i");
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
        FormatDBRtoValue(count, char,  &(cval->value), dbr_char_t,        "b");
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
        FormatDBRtoValue(count, long,  &(cval->value), dbr_long_t,        "i");
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
        FormatDBRtoValue(count, double,  &(cval->value), dbr_double_t,    "d");
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
