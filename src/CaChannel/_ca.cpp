
#include <Python.h>

#define epicsAlarmGLOBAL
#include <alarm.h>
#undef epicsAlarmGLOBAL
#include <cadef.h>

#ifdef WITH_NUMPY
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>
#endif

static bool has_numpy = false;

#ifndef MIN
     #define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#endif

#ifndef PyModule_AddIntMacro
    #define PyModule_AddIntMacro(m, c) PyModule_AddIntConstant(m, #c, c)
#endif


#if PY_MAJOR_VERSION >= 3
    #define PyInt_FromLong PyLong_FromLong
    #define PyString_Check PyUnicode_Check
    #define PyString_FromString PyUnicode_FromString
    #define PyString_AsString PyUnicode_AsUTF8
    #define CAPSULE_BUILD(ptr,name, destr) PyCapsule_New(ptr, name, destr)
    #define CAPSULE_CHECK(obj) PyCapsule_CheckExact(obj)
    #define CAPSULE_EXTRACT(obj,name) PyCapsule_GetPointer(obj, name)
#else
    #define PyBytes_Check PyString_Check
    #define PyBytes_FromString PyString_FromString
    #define PyBytes_AsString PyString_AsString
    #define CAPSULE_BUILD(ptr,name, destr) PyCObject_FromVoidPtr(ptr, destr)
    #define CAPSULE_CHECK(obj) PyCObject_Check(obj)
    #define CAPSULE_EXTRACT(obj,name) PyCObject_AsVoidPtr(obj)
#endif

static PyObject *Py_ca_create_context(PyObject *self, PyObject *args);
static PyObject *Py_ca_destroy_context(PyObject *self, PyObject *args);
static PyObject *Py_ca_attach_context(PyObject *self, PyObject *args);
static PyObject *Py_ca_detach_context(PyObject *self, PyObject *args);
static PyObject *Py_ca_current_context(PyObject *self, PyObject *args);
static PyObject *Py_ca_show_context(PyObject *self, PyObject *args, PyObject *kws);

static PyObject *Py_ca_create_channel(PyObject *self, PyObject *args, PyObject *kws);
static PyObject *Py_ca_clear_channel(PyObject *self, PyObject *args);
static PyObject *Py_ca_get(PyObject *self, PyObject *args, PyObject *kws);
static PyObject *Py_ca_put(PyObject *self, PyObject *args, PyObject *kws);
static PyObject *Py_ca_create_subscription(PyObject *self, PyObject *args, PyObject *kws);
static PyObject *Py_ca_clear_subscription(PyObject *self, PyObject *args);

static PyObject *Py_ca_replace_access_rights_event(PyObject *self, PyObject *args);
static PyObject *Py_ca_add_exception_event(PyObject *self, PyObject *args);
static PyObject *Py_ca_replace_printf_handler(PyObject *self, PyObject *args);

static PyObject *Py_ca_flush_io(PyObject *self, PyObject *args);
static PyObject *Py_ca_pend_io(PyObject *self, PyObject *args);
static PyObject *Py_ca_pend_event(PyObject *self, PyObject *args);
static PyObject *Py_ca_poll(PyObject *self, PyObject *args);
static PyObject *Py_ca_test_io(PyObject *self, PyObject *args);

static PyObject *Py_ca_field_type(PyObject *self, PyObject *args);
static PyObject *Py_ca_element_count(PyObject *self, PyObject *args);
static PyObject *Py_ca_name(PyObject *self, PyObject *args);
static PyObject *Py_ca_state(PyObject *self, PyObject *args);
static PyObject *Py_ca_host_name(PyObject *self, PyObject *args);
static PyObject *Py_ca_read_access(PyObject *self, PyObject *args);
static PyObject *Py_ca_write_access(PyObject *self, PyObject *args);

static PyObject *Py_ca_sg_create(PyObject *self, PyObject *args);
static PyObject *Py_ca_sg_delete(PyObject *self, PyObject *args);
static PyObject *Py_ca_sg_get(PyObject *self, PyObject *args, PyObject *kws);
static PyObject *Py_ca_sg_put(PyObject *self, PyObject *args, PyObject *kws);
static PyObject *Py_ca_sg_test(PyObject *self, PyObject *args);
static PyObject *Py_ca_sg_reset(PyObject *self, PyObject *args);
static PyObject *Py_ca_sg_block(PyObject *self, PyObject *args);

static PyObject *Py_dbf_type_is_valid(PyObject *self, PyObject *args);
static PyObject *Py_dbf_type_to_DBR(PyObject *self, PyObject *args);
static PyObject *Py_dbf_type_to_DBR_STS(PyObject *self, PyObject *args);
static PyObject *Py_dbf_type_to_DBR_TIME(PyObject *self, PyObject *args);
static PyObject *Py_dbf_type_to_DBR_GR(PyObject *self, PyObject *args);
static PyObject *Py_dbf_type_to_DBR_CTRL(PyObject *self, PyObject *args);

static PyObject *Py_dbr_type_is_valid(PyObject *self, PyObject *args);
static PyObject *Py_dbr_type_is_plain(PyObject *self, PyObject *args);
static PyObject *Py_dbr_type_is_STS(PyObject *self, PyObject *args);
static PyObject *Py_dbr_type_is_TIME(PyObject *self, PyObject *args);
static PyObject *Py_dbr_type_is_GR(PyObject *self, PyObject *args);
static PyObject *Py_dbr_type_is_CTRL(PyObject *self, PyObject *args);
static PyObject *Py_dbr_type_is_STRING(PyObject *self, PyObject *args);
static PyObject *Py_dbr_type_is_SHORT(PyObject *self, PyObject *args);
static PyObject *Py_dbr_type_is_FLOAT(PyObject *self, PyObject *args);
static PyObject *Py_dbr_type_is_ENUM(PyObject *self, PyObject *args);
static PyObject *Py_dbr_type_is_CHAR(PyObject *self, PyObject *args);
static PyObject *Py_dbr_type_is_LONG(PyObject *self, PyObject *args);
static PyObject *Py_dbr_type_is_DOUBLE(PyObject *self, PyObject *args);

static PyObject *Py_dbf_text(PyObject *self, PyObject *args);
static PyObject *Py_dbr_text(PyObject *self, PyObject *args);
static PyObject *Py_dbf_text_to_type(PyObject *self, PyObject *args);
static PyObject *Py_dbr_text_to_type(PyObject *self, PyObject *args);
static PyObject *Py_ca_message(PyObject *self, PyObject *args);
static PyObject *Py_alarmSeverityString(PyObject *self, PyObject *args);
static PyObject *Py_alarmStatusString(PyObject *self, PyObject *args);

static PyObject *CBufferToPythonDict(chtype type, unsigned long count, const void *val, int use_numpy);
static void *setup_put(chanId chid, PyObject *pValue, PyObject *pType, PyObject *pCount,
                       chtype &dbrtype, unsigned long &count);

/********************************************
 *          DBRValue object type            *
 ********************************************/
/*
This object type holds the data storage for ca_get and ca_sg_get function.
Only after ca_pend_io returns ECA_NORMAL, call `get` method to unpack the value.
*/
typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
    chtype dbrtype;
    unsigned long count;
    void *dbr;
    bool use_numpy;
} DBRValueObject;

static void DBRValue_dealloc(DBRValueObject* self)
{
    if (self->dbr)
        free(self->dbr);

    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *DBRValue_get(DBRValueObject *self)
{
    if (self->dbr == NULL) {
        PyErr_SetString(PyExc_ValueError, "DBRValue_get called with null pointer");
        return NULL;
    }
    PyObject *value = CBufferToPythonDict(self->dbrtype, self->count, self->dbr, self->use_numpy);

    return value;
}

static PyObject* DBRValue_getattro(DBRValueObject *self, PyObject* name)
{
    PyObject *pResult = NULL;
	char* attr = PyString_AsString(name);
    if (strcmp(attr, "use_numpy") == 0) {
        pResult = Py_BuildValue("i", self->use_numpy);
    } else {
		pResult = PyObject_GenericGetAttr((PyObject*)self, name);
    }
    return pResult;
}

static int DBRValue_setattro(DBRValueObject *self, PyObject* name, PyObject* value)
{
    int error = 0;
	char* attr = PyString_AsString(name);
    if (strcmp(attr, "use_numpy") == 0) {
        self->use_numpy = (PyLong_AsLong(value) != 0);
    } else {
		error = PyObject_GenericSetAttr((PyObject*)self, name, value);
    }
    return error;
}

static PyMethodDef DBRValue_methods[] = {
    {"get", (PyCFunction)DBRValue_get, METH_NOARGS, "Get as python object"},
    {NULL, NULL, 0, NULL}
};

static PyTypeObject DBRValueType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "ca.DBRValue",             /*tp_name*/
    sizeof(DBRValueObject),    /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)DBRValue_dealloc,/*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    (getattrofunc)DBRValue_getattro,/*tp_getattro*/
    (setattrofunc)DBRValue_setattro,/*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "DBRValue object",         /* tp_doc */
    0,		                   /* tp_traverse */
    0,		                   /* tp_clear */
    0,		                   /* tp_richcompare */
    0,		                   /* tp_weaklistoffset */
    0,		                   /* tp_iter */
    0,		                   /* tp_iternext */
    DBRValue_methods,          /* tp_methods */
};

static PyObject *DBRValue_New(chtype dbrtype, unsigned long count, void *dbr, bool use_numpy)
{
    DBRValueObject *self;

    self = PyObject_NEW(DBRValueObject, &DBRValueType);
    if (self == NULL) {
        return NULL;
    }

    self->dbrtype = dbrtype;
    self->count = count;
    self->dbr = dbr;
    self->use_numpy = use_numpy;

    return (PyObject *) self;
}

/*  Method Table  */
static PyMethodDef CA_Methods[] = {
    /* CA context */
    {"create_context",      Py_ca_create_context,   METH_VARARGS, "Create a CA context"},
    {"destroy_context",     Py_ca_destroy_context,  METH_VARARGS, "Destroy a CA context"},
    {"attach_context",      Py_ca_attach_context,   METH_VARARGS, "Detach a CA context"},
    {"detach_context",      Py_ca_detach_context,   METH_VARARGS, "Attach to a CA context"},
    {"current_context",     Py_ca_current_context,  METH_VARARGS, "Get the current CA context"},
    {"show_context", (PyCFunction)Py_ca_show_context,     METH_VARARGS|METH_KEYWORDS, "Show the CA context information"},
    /* Channel creation */
    {"create_channel", (PyCFunction)Py_ca_create_channel,   METH_VARARGS|METH_KEYWORDS, "Create a CA channel connection"},
    {"clear_channel",       Py_ca_clear_channel,    METH_VARARGS, "Shutdown a CA channel connection"},
    {"get",          (PyCFunction)Py_ca_get,              METH_VARARGS|METH_KEYWORDS, "Read PV's value"},
    {"put",          (PyCFunction)Py_ca_put,              METH_VARARGS|METH_KEYWORDS, "Write a value to PV"},
    {"create_subscription", (PyCFunction)Py_ca_create_subscription,METH_VARARGS|METH_KEYWORDS,"Subscribe for state changes"},
    {"clear_subscription",  Py_ca_clear_subscription, METH_VARARGS,"Unsubscribe for state changes"},
    {"replace_access_rights_event", Py_ca_replace_access_rights_event, METH_VARARGS, "Replace access right event"},
    {"add_exception_event", Py_ca_add_exception_event, METH_VARARGS, "Replace exception event handler"},
    {"replace_printf_handler", Py_ca_replace_printf_handler, METH_VARARGS, "Replace printf handler"},
    /* Info */
    {"field_type",      Py_ca_field_type,       METH_VARARGS, "PV's native type"},
    {"element_count",   Py_ca_element_count,    METH_VARARGS, "PV's array element count"},
    {"name",            Py_ca_name,             METH_VARARGS, "PV's name"},
    {"state",           Py_ca_state,            METH_VARARGS, "State of the CA channel connection"},
    {"host_name",       Py_ca_host_name,        METH_VARARGS, "Host to which the channel is connected"},
    {"read_access",     Py_ca_read_access,      METH_VARARGS, "PV's readability"},
    {"write_access",    Py_ca_write_access,     METH_VARARGS, "PV's writability"},
    /* Execution */
    {"flush_io",    Py_ca_flush_io,     METH_VARARGS, "flush IO requests"},
    {"pend_io",     Py_ca_pend_io,      METH_VARARGS, "wait pending connection and get"},
    {"pend_event",  Py_ca_pend_event,   METH_VARARGS, "process background activities"},
    {"poll",        Py_ca_poll,         METH_VARARGS, "process background activities for 1e-12s"},
    {"test_io",     Py_ca_test_io,      METH_VARARGS, "check pending connection and get"},
    {"sg_create",   Py_ca_sg_create,    METH_VARARGS, "Create a synchronous group"},
    {"sg_delete",   Py_ca_sg_delete,    METH_VARARGS, "Delete a synchronous group"},
    {"sg_get", (PyCFunction)Py_ca_sg_get,  METH_VARARGS|METH_KEYWORDS, "Read PV's value"},
    {"sg_put", (PyCFunction)Py_ca_sg_put,  METH_VARARGS|METH_KEYWORDS, "Write a value to PV"},
    {"sg_reset",    Py_ca_sg_reset,     METH_VARARGS, "Reset outstanding requests"},
    {"sg_block",    Py_ca_sg_block,     METH_VARARGS, "wait pending get"},
    {"sg_test",     Py_ca_sg_test,      METH_VARARGS, "check pending get"},

    /* Utility */
    {"alarmSeverityString",    Py_alarmSeverityString,    METH_VARARGS, "alarm severity message"},
    {"alarmStatusString",      Py_alarmStatusString,      METH_VARARGS, "alarm status message"},
    {"dbf_type_to_text",    Py_dbf_text,    METH_VARARGS, "field type name"},
    {"dbf_text",            Py_dbf_text,    METH_VARARGS, "field type name"},
    {"dbf_text_to_type",    Py_dbf_text_to_type,    METH_VARARGS, "field type name to type"},
    {"dbr_type_to_text",    Py_dbr_text,    METH_VARARGS, "request type name"},
    {"dbr_text",            Py_dbr_text,    METH_VARARGS, "request type name"},
    {"dbr_text_to_type",    Py_dbr_text_to_type,    METH_VARARGS, "request type name to type"},
    {"message",     Py_ca_message,  METH_VARARGS, "status error message"},
    {"dbf_type_is_valid",   Py_dbf_type_is_valid,   METH_VARARGS, "dbf_type_is_valid"},
    {"dbf_type_to_DBR",     Py_dbf_type_to_DBR,     METH_VARARGS, "dbf_type_to_DBR"},
    {"dbf_type_to_DBR_STS", Py_dbf_type_to_DBR_STS, METH_VARARGS, "dbf_type_to_DBR_STS"},
    {"dbf_type_to_DBR_TIME",Py_dbf_type_to_DBR_TIME,METH_VARARGS, "dbf_type_to_DBR_TIME"},
    {"dbf_type_to_DBR_GR",  Py_dbf_type_to_DBR_GR,  METH_VARARGS, "dbf_type_to_DBR_GR"},
    {"dbf_type_to_DBR_CTRL",Py_dbf_type_to_DBR_CTRL,METH_VARARGS, "dbf_type_to_DBR_CTRL"},
    {"dbr_type_is_valid",   Py_dbr_type_is_valid,   METH_VARARGS, "dbr_type_is_valid"},
    {"dbr_type_is_plain",   Py_dbr_type_is_plain,   METH_VARARGS, "dbr_type_is_plain"},
    {"dbr_type_is_STS",     Py_dbr_type_is_STS,     METH_VARARGS, "dbr_type_is_STS"},
    {"dbr_type_is_TIME",    Py_dbr_type_is_TIME,    METH_VARARGS, "dbr_type_is_TIME"},
    {"dbr_type_is_GR",      Py_dbr_type_is_GR,      METH_VARARGS, "dbr_type_is_GR"},
    {"dbr_type_is_CTRL",    Py_dbr_type_is_CTRL,    METH_VARARGS, "dbr_type_is_CTRL"},
    {"dbr_type_is_STRING",  Py_dbr_type_is_STRING,  METH_VARARGS, "dbr_type_is_STRING"},
    {"dbr_type_is_SHORT",   Py_dbr_type_is_SHORT,   METH_VARARGS, "dbr_type_is_SHORT"},
    {"dbr_type_is_FLOAT",   Py_dbr_type_is_FLOAT,   METH_VARARGS, "dbr_type_is_FLOAT"},
    {"dbr_type_is_ENUM",    Py_dbr_type_is_ENUM,    METH_VARARGS, "dbr_type_is_ENUM"},
    {"dbr_type_is_CHAR",    Py_dbr_type_is_CHAR,    METH_VARARGS, "dbr_type_is_CHAR"},
    {"dbr_type_is_LONG",    Py_dbr_type_is_LONG,    METH_VARARGS, "dbr_type_is_LONG"},
    {"dbr_type_is_DOUBLE",  Py_dbr_type_is_DOUBLE,  METH_VARARGS, "dbr_type_is_DOUBLE"},
    {NULL, NULL, 0, NULL}
};

#if PY_MAJOR_VERSION >= 3
static struct PyModuleDef CA_Module = {
  PyModuleDef_HEAD_INIT,
  "_ca", /* name */
  "python binding to channel access library", /* doc */
  -1, /* size */
  CA_Methods, /* methods */
  NULL, /* reload */
  NULL, /* traverse */
  NULL, /* clear */
  NULL, /* free */
};
#endif

#if PY_MAJOR_VERSION >= 3
    #define MOD_INIT(name) PyMODINIT_FUNC PyInit_##name(void)
#else
    #define MOD_INIT(name) extern "C" void init##name(void)
#endif

/* entry point for Python module initializer */

MOD_INIT(_ca) {
    PyObject *pModule;

    if(!PyEval_ThreadsInitialized()) {
      PyEval_InitThreads();
    }

    #if PY_MAJOR_VERSION >= 3
    pModule=PyModule_Create(&CA_Module);
    #else
    pModule=Py_InitModule("_ca", CA_Methods);
    #endif

    PyType_Ready(&DBRValueType);

    #ifdef WITH_NUMPY
    if (_import_array() < 0) {
        PyErr_Clear();
    } else {
        has_numpy = true;
    }
    PyModule_AddIntConstant(pModule, "WITH_NUMPY", 1);
    #else
    PyModule_AddIntConstant(pModule, "WITH_NUMPY", 0);
    #endif
    PyModule_AddIntConstant(pModule, "HAS_NUMPY", has_numpy);

    PyModule_AddIntMacro(pModule, DBF_STRING);
    PyModule_AddIntMacro(pModule, DBF_SHORT);
    PyModule_AddIntMacro(pModule, DBF_INT);
    PyModule_AddIntMacro(pModule, DBF_FLOAT);
    PyModule_AddIntMacro(pModule, DBF_ENUM);
    PyModule_AddIntMacro(pModule, DBF_CHAR);
    PyModule_AddIntMacro(pModule, DBF_LONG);
    PyModule_AddIntMacro(pModule, DBF_DOUBLE);

    PyModule_AddIntMacro(pModule, DBR_STRING);
    PyModule_AddIntMacro(pModule, DBR_SHORT);
    PyModule_AddIntMacro(pModule, DBR_INT);
    PyModule_AddIntMacro(pModule, DBR_FLOAT);
    PyModule_AddIntMacro(pModule, DBR_ENUM);
    PyModule_AddIntMacro(pModule, DBR_CHAR);
    PyModule_AddIntMacro(pModule, DBR_LONG);
    PyModule_AddIntMacro(pModule, DBR_DOUBLE);

    PyModule_AddIntMacro(pModule, DBR_STS_STRING);
    PyModule_AddIntMacro(pModule, DBR_STS_SHORT);
    PyModule_AddIntMacro(pModule, DBR_STS_INT);
    PyModule_AddIntMacro(pModule, DBR_STS_FLOAT);
    PyModule_AddIntMacro(pModule, DBR_STS_ENUM);
    PyModule_AddIntMacro(pModule, DBR_STS_CHAR);
    PyModule_AddIntMacro(pModule, DBR_STS_LONG);
    PyModule_AddIntMacro(pModule, DBR_STS_DOUBLE);

    PyModule_AddIntMacro(pModule, DBR_TIME_STRING);
    PyModule_AddIntMacro(pModule, DBR_TIME_SHORT);
    PyModule_AddIntMacro(pModule, DBR_TIME_INT);
    PyModule_AddIntMacro(pModule, DBR_TIME_FLOAT);
    PyModule_AddIntMacro(pModule, DBR_TIME_ENUM);
    PyModule_AddIntMacro(pModule, DBR_TIME_CHAR);
    PyModule_AddIntMacro(pModule, DBR_TIME_LONG);
    PyModule_AddIntMacro(pModule, DBR_TIME_DOUBLE);

    PyModule_AddIntMacro(pModule, DBR_GR_STRING);
    PyModule_AddIntMacro(pModule, DBR_GR_SHORT);
    PyModule_AddIntMacro(pModule, DBR_GR_INT);
    PyModule_AddIntMacro(pModule, DBR_GR_FLOAT);
    PyModule_AddIntMacro(pModule, DBR_GR_ENUM);
    PyModule_AddIntMacro(pModule, DBR_GR_CHAR);
    PyModule_AddIntMacro(pModule, DBR_GR_LONG);
    PyModule_AddIntMacro(pModule, DBR_GR_DOUBLE);


    PyModule_AddIntMacro(pModule, DBR_CTRL_STRING);
    PyModule_AddIntMacro(pModule, DBR_CTRL_SHORT);
    PyModule_AddIntMacro(pModule, DBR_CTRL_INT);
    PyModule_AddIntMacro(pModule, DBR_CTRL_FLOAT);
    PyModule_AddIntMacro(pModule, DBR_CTRL_ENUM);
    PyModule_AddIntMacro(pModule, DBR_CTRL_CHAR);
    PyModule_AddIntMacro(pModule, DBR_CTRL_LONG);
    PyModule_AddIntMacro(pModule, DBR_CTRL_DOUBLE);

    PyModule_AddIntMacro(pModule, DBR_PUT_ACKT);
    PyModule_AddIntMacro(pModule, DBR_PUT_ACKS);
    PyModule_AddIntMacro(pModule, DBR_STSACK_STRING);
    PyModule_AddIntMacro(pModule, DBR_CLASS_NAME);
    PyModule_AddIntMacro(pModule, LAST_BUFFER_TYPE);

    PyModule_AddIntMacro(pModule, ECA_NORMAL);
    PyModule_AddIntMacro(pModule, ECA_MAXIOC);        
    PyModule_AddIntMacro(pModule, ECA_UKNHOST);     
    PyModule_AddIntMacro(pModule, ECA_UKNSERV);       
    PyModule_AddIntMacro(pModule, ECA_SOCK);          
    PyModule_AddIntMacro(pModule, ECA_CONN);          
    PyModule_AddIntMacro(pModule, ECA_ALLOCMEM);      
    PyModule_AddIntMacro(pModule, ECA_UKNCHAN);       
    PyModule_AddIntMacro(pModule, ECA_UKNFIELD);
    PyModule_AddIntMacro(pModule, ECA_TOLARGE);
    PyModule_AddIntMacro(pModule, ECA_TIMEOUT);
    PyModule_AddIntMacro(pModule, ECA_NOSUPPORT);
    PyModule_AddIntMacro(pModule, ECA_STRTOBIG);
    PyModule_AddIntMacro(pModule, ECA_DISCONNCHID);
    PyModule_AddIntMacro(pModule, ECA_BADTYPE);
    PyModule_AddIntMacro(pModule, ECA_CHIDNOTFND);
    PyModule_AddIntMacro(pModule, ECA_CHIDRETRY);
    PyModule_AddIntMacro(pModule, ECA_INTERNAL);
    PyModule_AddIntMacro(pModule, ECA_DBLCLFAIL);
    PyModule_AddIntMacro(pModule, ECA_GETFAIL);
    PyModule_AddIntMacro(pModule, ECA_PUTFAIL);
    PyModule_AddIntMacro(pModule, ECA_ADDFAIL);
    PyModule_AddIntMacro(pModule, ECA_BADCOUNT);
    PyModule_AddIntMacro(pModule, ECA_BADSTR);
    PyModule_AddIntMacro(pModule, ECA_DISCONN);
    PyModule_AddIntMacro(pModule, ECA_DBLCHNL);
    PyModule_AddIntMacro(pModule, ECA_EVDISALLOW);
    PyModule_AddIntMacro(pModule, ECA_BUILDGET);
    PyModule_AddIntMacro(pModule, ECA_NEEDSFP);
    PyModule_AddIntMacro(pModule, ECA_OVEVFAIL);
    PyModule_AddIntMacro(pModule, ECA_BADMONID);
    PyModule_AddIntMacro(pModule, ECA_NEWADDR);
    PyModule_AddIntMacro(pModule, ECA_NEWCONN);
    PyModule_AddIntMacro(pModule, ECA_NOCACTX);
    PyModule_AddIntMacro(pModule, ECA_DEFUNCT);
    PyModule_AddIntMacro(pModule, ECA_EMPTYSTR);
    PyModule_AddIntMacro(pModule, ECA_NOREPEATER);
    PyModule_AddIntMacro(pModule, ECA_NOCHANMSG);
    PyModule_AddIntMacro(pModule, ECA_DLCKREST);
    PyModule_AddIntMacro(pModule, ECA_SERVBEHIND);
    PyModule_AddIntMacro(pModule, ECA_NOCAST);
    PyModule_AddIntMacro(pModule, ECA_BADMASK);
    PyModule_AddIntMacro(pModule, ECA_IODONE);
    PyModule_AddIntMacro(pModule, ECA_IOINPROGRESS);
    PyModule_AddIntMacro(pModule, ECA_BADSYNCGRP);
    PyModule_AddIntMacro(pModule, ECA_PUTCBINPROG);
    PyModule_AddIntMacro(pModule, ECA_NORDACCESS);
    PyModule_AddIntMacro(pModule, ECA_NOWTACCESS);
    PyModule_AddIntMacro(pModule, ECA_ANACHRONISM);
    PyModule_AddIntMacro(pModule, ECA_NOSEARCHADDR);
    PyModule_AddIntMacro(pModule, ECA_NOCONVERT);
    PyModule_AddIntMacro(pModule, ECA_BADCHID);
    PyModule_AddIntMacro(pModule, ECA_BADFUNCPTR);
    PyModule_AddIntMacro(pModule, ECA_ISATTACHED);
    PyModule_AddIntMacro(pModule, ECA_UNAVAILINSERV);
    PyModule_AddIntMacro(pModule, ECA_CHANDESTROY);
    PyModule_AddIntMacro(pModule, ECA_BADPRIORITY);
    PyModule_AddIntMacro(pModule, ECA_NOTTHREADED);
    PyModule_AddIntMacro(pModule, ECA_16KARRAYCLIENT);
    PyModule_AddIntMacro(pModule, ECA_CONNSEQTMO);
    PyModule_AddIntMacro(pModule, ECA_UNRESPTMO);

    PyModule_AddIntMacro(pModule, DBE_VALUE);
    PyModule_AddIntMacro(pModule, DBE_ARCHIVE);
    PyModule_AddIntMacro(pModule, DBE_LOG);
    PyModule_AddIntMacro(pModule, DBE_ALARM);
    PyModule_AddIntMacro(pModule, DBE_PROPERTY);

    PyModule_AddIntMacro(pModule, CA_OP_CONN_UP);
    PyModule_AddIntMacro(pModule, CA_OP_CONN_DOWN);

    PyModule_AddIntMacro(pModule, cs_never_conn);
    PyModule_AddIntMacro(pModule, cs_prev_conn);
    PyModule_AddIntMacro(pModule, cs_conn);
    PyModule_AddIntMacro(pModule, cs_closed);
    PyModule_AddIntConstant(pModule, "cs_never_search", 4);

    PyModule_AddIntMacro(pModule, NO_ALARM);
    PyModule_AddIntMacro(pModule, MINOR_ALARM);
    PyModule_AddIntMacro(pModule, MAJOR_ALARM);
    PyModule_AddIntMacro(pModule, INVALID_ALARM);

    PyModule_AddIntMacro(pModule, READ_ALARM);
    PyModule_AddIntMacro(pModule, WRITE_ALARM);
    PyModule_AddIntMacro(pModule, HIHI_ALARM);
    PyModule_AddIntMacro(pModule, HIGH_ALARM);
    PyModule_AddIntMacro(pModule, LOLO_ALARM);
    PyModule_AddIntMacro(pModule, LOW_ALARM);
    PyModule_AddIntMacro(pModule, STATE_ALARM);
    PyModule_AddIntMacro(pModule, COS_ALARM);
    PyModule_AddIntMacro(pModule, COMM_ALARM);
    PyModule_AddIntMacro(pModule, TIMEOUT_ALARM);
    PyModule_AddIntMacro(pModule, HW_LIMIT_ALARM);
    PyModule_AddIntMacro(pModule, CALC_ALARM);
    PyModule_AddIntMacro(pModule, SCAN_ALARM);
    PyModule_AddIntMacro(pModule, LINK_ALARM);
    PyModule_AddIntMacro(pModule, SOFT_ALARM);
    PyModule_AddIntMacro(pModule, BAD_SUB_ALARM);
    PyModule_AddIntMacro(pModule, UDF_ALARM);
    PyModule_AddIntMacro(pModule, DISABLE_ALARM);
    PyModule_AddIntMacro(pModule, SIMM_ALARM);
    PyModule_AddIntMacro(pModule, READ_ACCESS_ALARM);
    PyModule_AddIntMacro(pModule, WRITE_ACCESS_ALARM);

    PyModule_AddIntMacro(pModule, POSIX_TIME_AT_EPICS_EPOCH);

    #if PY_MAJOR_VERSION >= 3
    return pModule;
    #endif
}

/*******************************************************
 *                    CA Context                       *
 *******************************************************/

static PyObject *Py_ca_create_context(PyObject *self, PyObject *args)
{
    int preemptive_callback = 1;
    if(!PyArg_ParseTuple(args, "|i", &preemptive_callback))
        return NULL;

    int status;

    Py_BEGIN_ALLOW_THREADS
    status = ca_context_create(preemptive_callback ?
                ca_enable_preemptive_callback : ca_disable_preemptive_callback);
    Py_END_ALLOW_THREADS

    return Py_BuildValue("i", status);
}

static PyObject *Py_ca_destroy_context(PyObject *self, PyObject *args)
{
    Py_BEGIN_ALLOW_THREADS
    ca_context_destroy();
    Py_END_ALLOW_THREADS

    Py_RETURN_NONE;
}

static PyObject *Py_ca_attach_context(PyObject *self, PyObject *args)
{
    PyObject *pObject;
    if(!PyArg_ParseTuple(args, "O", &pObject))
        return NULL;

    struct ca_client_context *pContext = (ca_client_context *) CAPSULE_EXTRACT(pObject, "ca_client_context");
    if (pContext == NULL)
        return NULL;

    int status;

    Py_BEGIN_ALLOW_THREADS
    status = ca_attach_context(pContext);
    Py_END_ALLOW_THREADS

    return Py_BuildValue("i", status);
}

static PyObject *Py_ca_detach_context(PyObject *self, PyObject *args)
{
    Py_BEGIN_ALLOW_THREADS
    ca_detach_context();
    Py_END_ALLOW_THREADS

    Py_RETURN_NONE;
}

static PyObject *Py_ca_current_context(PyObject *self, PyObject *args)
{
    struct ca_client_context *pContext;
    Py_BEGIN_ALLOW_THREADS
    pContext = ca_current_context();
    Py_END_ALLOW_THREADS

    if (pContext == NULL)
        Py_RETURN_NONE;
    else
        return CAPSULE_BUILD(pContext, "ca_client_context", NULL);
}

static PyObject *Py_ca_show_context(PyObject *self, PyObject *args, PyObject *kws)
{
    PyObject *pObject = NULL;
    int level = 0;

    const char *kwlist[] = {"context", "level",  NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kws, "|Oi", (char **)kwlist, &pObject, &level))
        return NULL;

    struct ca_client_context *pContext = NULL;
    if(pObject == NULL) {
        Py_BEGIN_ALLOW_THREADS
        pContext = ca_current_context();
        Py_END_ALLOW_THREADS
    }
    else
        pContext = (struct ca_client_context *)CAPSULE_EXTRACT(pObject, "ca_client_context");

    if (pContext != NULL) {
        Py_BEGIN_ALLOW_THREADS
        ca_context_status(pContext, level);
        Py_END_ALLOW_THREADS
    }

    Py_RETURN_NONE;
}



/*******************************************************
 *                    CA Channel                       *
 *******************************************************/

/*
    Class to store user supplied callback function and argument objects.
    It is used in operations ca_create_channel, ca_get_callback, ca_put_callback and ca_create_subscription
*/
class ChannelData {
public:
    ChannelData(PyObject *pCallback) : pAccessEventCallback(NULL), use_numpy(false) {
        this->pCallback = pCallback;
        Py_XINCREF(pCallback);
    }
    ~ChannelData() {
        Py_XDECREF(pCallback);
        Py_XDECREF(pAccessEventCallback);
    }

    PyObject *pCallback;
    evid eventID;
    PyObject *pAccessEventCallback;
    bool use_numpy;
};

static void connection_callback(struct connection_handler_args args)
{
    PyGILState_STATE gstate = PyGILState_Ensure();

    ChannelData *pData = (ChannelData *) ca_puser(args.chid);

    if(PyCallable_Check(pData->pCallback)) {
        PyObject *pChid = CAPSULE_BUILD(args.chid, "chid", NULL);
        PyObject *pArgs = Py_BuildValue("({s:O,s:i})", "chid", pChid, "op", args.op);

        PyObject *ret = PyObject_CallObject(pData->pCallback, pArgs);
        if (ret == NULL) {
            PyErr_Print();
        }
        Py_XDECREF(ret);
	    Py_XDECREF(pArgs);
	    Py_XDECREF(pChid);
    }

    PyGILState_Release(gstate);
}

static PyObject *Py_ca_create_channel(PyObject *self, PyObject *args, PyObject *kws)
{
    char *pName;
    PyObject *pCallback = NULL;
    int priority = 0;
    const char *kwlist[] = {"name", "callback", "priority",  NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kws, "z|Oi", (char **)kwlist, &pName, &pCallback, &priority))
        return NULL;

    chanId chid = NULL;
    int status;

    ChannelData *pData = new ChannelData(pCallback);
    if(PyCallable_Check(pCallback)) {
        Py_BEGIN_ALLOW_THREADS
        status = ca_create_channel(pName, &connection_callback, pData, priority, &chid);
        Py_END_ALLOW_THREADS
        if (status != ECA_NORMAL) delete pData;
    } else {
        Py_BEGIN_ALLOW_THREADS
        status = ca_create_channel(pName, NULL, pData, priority, &chid);
        Py_END_ALLOW_THREADS
    }

    if (status == ECA_NORMAL) {
        PyObject *pChid = CAPSULE_BUILD(chid, "chid", NULL);
        PyObject *pRes = Py_BuildValue("iO", status, pChid);
        Py_XDECREF(pChid);
        return pRes;
    } else {
        return Py_BuildValue("iO", status, Py_None);
    }
}

static PyObject *Py_ca_clear_channel(PyObject *self, PyObject *args)
{
    PyObject *pChid;
    if(!PyArg_ParseTuple(args, "O", &pChid))
        return NULL;

    chanId chid = (chanId) CAPSULE_EXTRACT(pChid, "chid");
    if (chid == NULL)
        return NULL;


    int status;

    Py_BEGIN_ALLOW_THREADS
    ChannelData *pData = (ChannelData *) ca_puser(chid);
    status = ca_clear_channel(chid);
    delete pData;
    Py_END_ALLOW_THREADS

    return Py_BuildValue("i", status);
}


/*******************************************************
 *                    CA Operation                     *
 *******************************************************/

static void get_callback(struct event_handler_args args)
{
    PyGILState_STATE gstate = PyGILState_Ensure();

    ChannelData *pData= (ChannelData *)args.usr;

    if (PyCallable_Check(pData->pCallback)) {
        PyObject *pChid = CAPSULE_BUILD(args.chid, "chid", NULL);
        PyObject *pValue = CBufferToPythonDict(args.type,
                    args.count,
                    args.dbr,
                    pData->use_numpy);
        PyObject *pArgs = Py_BuildValue(
            "({s:O,s:i,s:i,s:i,s:O})",
            "chid", pChid,
            "type", args.type,
            "count", args.count,
            "status", args.status,
            "value", pValue
        );
        PyObject *ret = PyObject_CallObject(pData->pCallback, pArgs);
        if (ret == NULL) {
            PyErr_Print();
        }
        Py_XDECREF(ret);
        Py_XDECREF(pValue);
        Py_XDECREF(pChid);
        Py_XDECREF(pArgs);
    }

    delete pData;

    PyGILState_Release(gstate);
}

static void event_callback(struct event_handler_args args)
{
    PyGILState_STATE gstate = PyGILState_Ensure();

    ChannelData *pData= (ChannelData *)args.usr;

    if (PyCallable_Check(pData->pCallback)) {
        PyObject *pChid = CAPSULE_BUILD(args.chid, "chid", NULL);
        PyObject *pValue = CBufferToPythonDict(args.type,
                    args.count,
                    args.dbr,
                    pData->use_numpy);
        PyObject *pArgs = Py_BuildValue(
            "({s:O,s:i,s:i,s:i,s:O})",
            "chid", pChid,
            "type", args.type,
            "count", args.count,
            "status", args.status,
            "value", pValue
        );
        PyObject *ret = PyObject_CallObject(pData->pCallback, pArgs);
        if (ret == NULL) {
            PyErr_Print();
        }
        Py_XDECREF(ret);
        Py_XDECREF(pValue);
        Py_XDECREF(pChid);
        Py_XDECREF(pArgs);
    }

    PyGILState_Release(gstate);
}


static PyObject *Py_ca_get(PyObject *self, PyObject *args, PyObject *kws)
{
    PyObject *pChid;
    PyObject *pType = Py_None;
    chtype field_type = -1;
    unsigned long element_count = 0;
    chtype dbrtype = -1;
    PyObject *pCount = Py_None;
    unsigned long count = 0;
    PyObject *pCallback = Py_None;
    bool use_numpy = false;
    int status;

    const char *kwlist[] = {"chid", "chtype", "count", "callback", "use_numpy", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kws, "O|OOOb", (char **)kwlist, &pChid, &pType, &pCount, &pCallback, &use_numpy))
        return NULL;

    chanId chid = (chanId) CAPSULE_EXTRACT(pChid, "chid");
    if (chid == NULL)
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    field_type = ca_field_type(chid);
    element_count = ca_element_count(chid);
    Py_END_ALLOW_THREADS

    if (pType == Py_None)
        dbrtype = dbf_type_to_DBR(field_type);
    else
        dbrtype = PyLong_AsLong(pType);

    if (pCount == Py_None)
        count = element_count;
    else
        count = MIN(element_count, PyLong_AsUnsignedLong(pCount));

    if (PyCallable_Check(pCallback)) {
        ChannelData *pData = new ChannelData(pCallback);
        pData->use_numpy = use_numpy;
        Py_BEGIN_ALLOW_THREADS
        status = ca_array_get_callback(dbrtype, count, chid, get_callback, pData);
        Py_END_ALLOW_THREADS
        if (status != ECA_NORMAL) {
            delete pData;
        }
        return Py_BuildValue("(iO)", status, Py_None);
    } else {
        // prepare the storage
        void * pValue = malloc(dbr_size_n(dbrtype, (count==0 || count > element_count) ? element_count : count));
        Py_BEGIN_ALLOW_THREADS
        status = ca_array_get(dbrtype, count, chid, pValue);
        Py_END_ALLOW_THREADS
        if (status == ECA_NORMAL) {
            return Py_BuildValue("(iN)", status, DBRValue_New(dbrtype, count, pValue, use_numpy));
        } else {
            free(pValue);
            return Py_BuildValue("(iO)", status, Py_None);
        }
    }
}

static void put_callback(struct event_handler_args args)
{
    PyGILState_STATE gstate = PyGILState_Ensure();

    ChannelData *pData= (ChannelData *)args.usr;

    if (PyCallable_Check(pData->pCallback)) {
        PyObject *pChid = CAPSULE_BUILD(args.chid, "chid", NULL);
        PyObject *pArgs = Py_BuildValue(
            "({s:O,s:i,s:i,s:i})",
            "chid", pChid,
            "type", args.type,
            "count", args.count,
            "status", args.status
        );
        PyObject *ret = PyObject_CallObject(pData->pCallback, pArgs);
        if (ret == NULL) {
            PyErr_Print();
        }
        Py_XDECREF(ret);
        Py_XDECREF(pChid);
        Py_XDECREF(pArgs);
    }

    delete pData;

    PyGILState_Release(gstate);
}


static PyObject *Py_ca_put(PyObject *self, PyObject *args, PyObject *kws)
{
    PyObject *pChid;
    PyObject *pValue;
    PyObject *pType = Py_None;
    PyObject *pCount = Py_None;
    chtype dbrtype = -1;
    unsigned long count = 1;
    PyObject *pCallback = Py_None;
    void *pbuf = NULL;
    int status;

    const char *kwlist[] = {"chid", "value", "chtype", "count", "callback", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kws, "OO|OOO", (char **)kwlist, &pChid, &pValue, &pType, &pCount, &pCallback))
        return NULL;

    chanId chid = (chanId) CAPSULE_EXTRACT(pChid, "chid");
    if (chid == NULL)
        return NULL;

    pbuf = setup_put(chid, pValue, pType, pCount, dbrtype, count);
    if (pbuf == NULL)
        return NULL;

    if (PyCallable_Check(pCallback)) {
        ChannelData *pData = new ChannelData(pCallback);
        Py_BEGIN_ALLOW_THREADS
        status = ca_array_put_callback(dbrtype, count, chid, pbuf, put_callback, pData);
        Py_END_ALLOW_THREADS
        if (status != ECA_NORMAL)
            delete pData;
    } else {
        Py_BEGIN_ALLOW_THREADS
        status = ca_array_put(dbrtype, count, chid, pbuf);
        Py_END_ALLOW_THREADS
    }

    free(pbuf);

    return Py_BuildValue("i", status);
}


static PyObject *Py_ca_create_subscription(PyObject *self, PyObject *args, PyObject *kws)
{
    PyObject *pChid;
    PyObject *pCallback = NULL;
    PyObject *pType = Py_None;
    PyObject *pCount = Py_None;
    PyObject *pMask = Py_None;
    chtype field_type = -1;
    unsigned long element_count = 0;
    chtype dbrtype = -1;
    unsigned long count = 0;
    unsigned long mask = DBE_VALUE | DBE_ALARM;
    bool use_numpy = false;
    const char *kwlist[] = {"chid", "callback", "chtype", "count", "mask", "use_numpy", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kws, "OO|OOOb", (char **)kwlist, &pChid,  &pCallback, &pType, &pCount, &pMask, &use_numpy))
        return NULL;

    chanId chid = (chanId) CAPSULE_EXTRACT(pChid, "chid");
    if (chid == NULL)
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    field_type = ca_field_type(chid);
    element_count = ca_element_count(chid);
    Py_END_ALLOW_THREADS

    if (pType == Py_None)
        dbrtype = dbf_type_to_DBR(field_type);
    else
        dbrtype = PyLong_AsLong(pType);

    if (pCount == Py_None)
        count = element_count;
    else
        count = PyLong_AsUnsignedLong(pCount);

    if (pMask == Py_None)
        mask = DBE_VALUE | DBE_ALARM;
    else
        mask = PyLong_AsLong(pMask);

    ChannelData *pData = new ChannelData(pCallback);
    pData->use_numpy = use_numpy;

    evid eventID;
    int status;
    Py_BEGIN_ALLOW_THREADS
    status = ca_create_subscription(dbrtype, count, chid, mask, event_callback, pData, &eventID);
    Py_END_ALLOW_THREADS

    if (status == ECA_NORMAL) {
        pData->eventID = eventID;
        PyObject *pRes = Py_BuildValue("(iN)", status, CAPSULE_BUILD(pData, "evid", NULL));
        return pRes;
    } else {
        delete pData;
        return Py_BuildValue("(iO)", status, Py_None);
    }
}

static PyObject *Py_ca_clear_subscription(PyObject *self, PyObject *args)
{
    PyObject *pObject;
    if(!PyArg_ParseTuple(args, "O", &pObject))
        return NULL;

    ChannelData *pData = (ChannelData*) CAPSULE_EXTRACT(pObject, "evid");
    if (pData == NULL)
        return NULL;

    int status;

    Py_BEGIN_ALLOW_THREADS
    status = ca_clear_subscription(pData->eventID);
    Py_END_ALLOW_THREADS

    delete pData;

    return Py_BuildValue("i", status);
}

static void access_rights_handler(struct access_rights_handler_args args)
{
    PyGILState_STATE gstate = PyGILState_Ensure();

    ChannelData *pData= (ChannelData *)ca_puser(args.chid);

    if (PyCallable_Check(pData->pAccessEventCallback)) {
        PyObject *pChid = CAPSULE_BUILD(args.chid, "chid", NULL);
        PyObject *pArgs = Py_BuildValue(
            "({s:O,s:i,s:i})",
            "chid", pChid,
            "read_access", args.ar.read_access,
            "write_access", args.ar.write_access
        );
        PyObject *ret = PyObject_CallObject(pData->pAccessEventCallback, pArgs);
        if (ret == NULL) {
            PyErr_Print();
        }
        Py_XDECREF(ret);
        Py_XDECREF(pChid);
        Py_XDECREF(pArgs);
    }

    PyGILState_Release(gstate);
}

static PyObject *Py_ca_replace_access_rights_event(PyObject *self, PyObject *args)
{
    PyObject *pChid;
    PyObject *pCallback = NULL;
    if(!PyArg_ParseTuple(args, "O|O", &pChid,  &pCallback))
        return NULL;

    chanId chid = (chanId) CAPSULE_EXTRACT(pChid, "chid");
    if (chid == NULL)
        return NULL;

    ChannelData *pData;
    Py_BEGIN_ALLOW_THREADS
    pData= (ChannelData *)ca_puser(chid);
    Py_END_ALLOW_THREADS

    /* release previous callback */
    Py_XDECREF(pData->pAccessEventCallback);
    pData->pAccessEventCallback = NULL;

    caArh *handler = NULL;
    if (PyCallable_Check(pCallback)) {
        /* store callback */
        pData->pAccessEventCallback = pCallback;
        Py_XINCREF(pCallback);

        handler = access_rights_handler;
    }

    int status;
    Py_BEGIN_ALLOW_THREADS
    status = ca_replace_access_rights_event(chid, handler);
    Py_END_ALLOW_THREADS

    return Py_BuildValue("i", status);
}

static PyObject *pExceptionCallback = NULL;

static void exception_handler(struct exception_handler_args args)
{
    PyGILState_STATE gstate = PyGILState_Ensure();

    if (PyCallable_Check(pExceptionCallback)) {
        PyObject *pChid = Py_None;
        if (args.chid)
            pChid = CAPSULE_BUILD(args.chid, "chid", NULL);
        PyObject *pArgs = Py_BuildValue(
            "({s:O,s:i,s:i,s:i,s:i,s:s,s:s,s:i})",
            "chid", pChid,
            "type", args.type,
            "count", args.count,
            "state", args.stat,
            "op", args.op,
            "ctx", args.ctx,
            "file", args.pFile,
            "lineNo", args.lineNo
        );
        if (pArgs == NULL)
            PyErr_Print();
        PyObject *ret = PyObject_CallObject(pExceptionCallback, pArgs);
        if (ret == NULL) {
            PyErr_Print();
        }
        Py_XDECREF(ret);
        Py_XDECREF(pChid);
        Py_XDECREF(pArgs);
    }

    PyGILState_Release(gstate);
}

static PyObject *Py_ca_add_exception_event(PyObject *self, PyObject *args)
{
    PyObject *pCallback = NULL;
    if(!PyArg_ParseTuple(args, "|O", &pCallback))
        return NULL;

    /* release previous callback */
    Py_XDECREF(pExceptionCallback);
    pExceptionCallback = NULL;

    caExceptionHandler *handler = NULL;
    if (PyCallable_Check(pCallback)) {
        /* store callback */
        Py_XINCREF(pCallback);
        pExceptionCallback = pCallback;

        handler = exception_handler; 
    }

    int status;
    Py_BEGIN_ALLOW_THREADS
    status = ca_add_exception_event(handler, NULL);
    Py_END_ALLOW_THREADS

    return Py_BuildValue("i", status);
}

static PyObject *pPrintfHandler = NULL;
int printf_handler(const char *pFormat, va_list args)
{
    PyGILState_STATE gstate = PyGILState_Ensure();

    /* message body is limited to 1023 chars */
    char message[1024];
    vsnprintf(message, 1024, pFormat, args);

    if (PyCallable_Check(pPrintfHandler)) {
        PyObject *pArgs = Py_BuildValue(
        "(s)",
        message
        );
        PyObject *ret = PyObject_CallObject(pPrintfHandler, pArgs);
        if (ret == NULL) {
            PyErr_Print();
        }
        Py_XDECREF(ret);
        Py_XDECREF(pArgs);
    }

    PyGILState_Release(gstate);

    return 0;
}

static PyObject *Py_ca_replace_printf_handler(PyObject *self, PyObject *args)
{
    PyObject *pCallback = NULL;
    if(!PyArg_ParseTuple(args, "|O", &pCallback))
        return NULL;

    /* release previous callback/args */
    Py_XDECREF(pPrintfHandler);
    pPrintfHandler = NULL;

    /* store callback/args */
    caPrintfFunc *pFunc = NULL;
    if (PyCallable_Check(pCallback)) {
        Py_XINCREF(pCallback);
        pPrintfHandler = pCallback;

        pFunc = printf_handler;
    }

    int status;
    Py_BEGIN_ALLOW_THREADS
    status = ca_replace_printf_handler(pFunc);
    Py_END_ALLOW_THREADS

    return Py_BuildValue("i", status);
}

/*******************************************************
 *               CA Synchronous Group                  *
 *******************************************************/

static PyObject *Py_ca_sg_create(PyObject *self, PyObject *args)
{
    CA_SYNC_GID gid;
    int status;
    
    Py_BEGIN_ALLOW_THREADS
    status = ca_sg_create(&gid);
    Py_END_ALLOW_THREADS

    return Py_BuildValue("(il)", status, gid);
}

static PyObject *Py_ca_sg_delete(PyObject *self, PyObject *args)
{
    CA_SYNC_GID gid;
    if(!PyArg_ParseTuple(args, "l", &gid))
        return NULL;

    int status;
    Py_BEGIN_ALLOW_THREADS
    status = ca_sg_delete(gid);
    Py_END_ALLOW_THREADS

    return Py_BuildValue("i", status);
}

static PyObject *Py_ca_sg_get(PyObject *self, PyObject *args, PyObject *kws)
{
    CA_SYNC_GID gid;
    PyObject *pChid;
    PyObject *pType = Py_None;
    PyObject *pCount = Py_None;
    chtype field_type = -1;
    unsigned long element_count = 0;
    chtype dbrtype = -1;
    unsigned long count = 0;
    bool use_numpy = false;

    const char *kwlist[] = {"gid", "chid", "chtype", "count", "use_numpy", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kws, "lO|OOb", (char **)kwlist, &gid, &pChid, &pType, &pCount, &use_numpy))
        return NULL;

    chanId chid = (chanId) CAPSULE_EXTRACT(pChid, "chid");
    if (chid == NULL)
        return NULL;
    
    Py_BEGIN_ALLOW_THREADS
    field_type = ca_field_type(chid);
    element_count = ca_element_count(chid);
    Py_END_ALLOW_THREADS

    if (pType == Py_None)
        dbrtype = dbf_type_to_DBR(field_type);
    else
        dbrtype = PyLong_AsLong(pType);

    if (pCount == Py_None)
        count = element_count;
    else
        count = (unsigned long)PyLong_AsUnsignedLong(pCount);

    // prepare the storage
    void * pValue = malloc(dbr_size_n(dbrtype, (count==0 || count > element_count) ? element_count : count));
    int status;
    Py_BEGIN_ALLOW_THREADS
    status = ca_sg_array_get(gid, dbrtype, count, chid, pValue);
    Py_END_ALLOW_THREADS

    if (status == ECA_NORMAL) {
        return Py_BuildValue("(iN)", status, DBRValue_New(dbrtype, count, pValue, use_numpy));
    } else {
        free(pValue);
        return Py_BuildValue("(iO)", status, Py_None);
    }
}

static PyObject *Py_ca_sg_put(PyObject *self, PyObject *args, PyObject *kws)
{
    CA_SYNC_GID gid;
    PyObject *pChid;
    PyObject *pValue;
    PyObject *pType = Py_None;
    PyObject *pCount = Py_None;
    void *pbuf = NULL;
    chtype dbrtype = -1;
    unsigned long count = 1;
    int status;

    const char *kwlist[] = {"gid", "chid", "value", "chtype", "count", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kws, "lOO|OO", (char **)kwlist, &gid, &pChid, &pValue, &pType, &pCount))
        return NULL;

    chanId chid = (chanId) CAPSULE_EXTRACT(pChid, "chid");
    if (chid == NULL)
        return NULL;

    pbuf = setup_put(chid, pValue, pType, pCount, dbrtype, count);
    if (pbuf == NULL)
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    status = ca_sg_array_put(gid, dbrtype, count, chid, pbuf);
    Py_END_ALLOW_THREADS

    free(pbuf);

    return Py_BuildValue("i", status);
}

static PyObject *Py_ca_sg_reset(PyObject *self, PyObject *args)
{
    CA_SYNC_GID gid;
    int status;

    if(!PyArg_ParseTuple(args, "l", &gid))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    status = ca_sg_reset(gid);
    Py_END_ALLOW_THREADS

    return Py_BuildValue("i", status);
}

static PyObject *Py_ca_sg_block(PyObject *self, PyObject *args)
{
    CA_SYNC_GID gid;
    double timeout;
    int status;

    if(!PyArg_ParseTuple(args, "ld", &gid, &timeout))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    status = ca_sg_block(gid, timeout);
    Py_END_ALLOW_THREADS

    return Py_BuildValue("i", status);
}

static PyObject *Py_ca_sg_test(PyObject *self, PyObject *args)
{
    CA_SYNC_GID gid;
    int status;

    if(!PyArg_ParseTuple(args, "l", &gid))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    status = ca_sg_test(gid);
    Py_END_ALLOW_THREADS

    return Py_BuildValue("i", status);
}


/*******************************************************
 *                    CA Execution                     *
 *******************************************************/

static PyObject *Py_ca_flush_io(PyObject *self, PyObject *args)
{
    int status;

    Py_BEGIN_ALLOW_THREADS
    status = ca_flush_io();
    Py_END_ALLOW_THREADS

    return Py_BuildValue("i", status);
}

static PyObject *Py_ca_pend_io(PyObject *self, PyObject *args)
{
    double timeout;
    if(!PyArg_ParseTuple(args, "d", &timeout))
        return NULL;

    int status;

    Py_BEGIN_ALLOW_THREADS
    status = ca_pend_io(timeout);
    Py_END_ALLOW_THREADS

    return Py_BuildValue("i", status);
}

static PyObject *Py_ca_pend_event(PyObject *self, PyObject *args)
{
    double timeout;
    if(!PyArg_ParseTuple(args, "d", &timeout))
        return NULL;

    int status;

    Py_BEGIN_ALLOW_THREADS
    status = ca_pend_event(timeout);
    Py_END_ALLOW_THREADS

    return Py_BuildValue("i", status);
}

static PyObject *Py_ca_poll(PyObject *self, PyObject *args)
{
    int status;

    Py_BEGIN_ALLOW_THREADS
    status = ca_poll();
    Py_END_ALLOW_THREADS

    return Py_BuildValue("i", status);
}

static PyObject *Py_ca_test_io(PyObject *self, PyObject *args)
{
    int status;

    Py_BEGIN_ALLOW_THREADS
    status = ca_test_io();
    Py_END_ALLOW_THREADS

    return Py_BuildValue("i", status);
}


/*******************************************************
 *                      CA Info                        *
 *******************************************************/

static PyObject *Py_ca_field_type(PyObject *self, PyObject *args)
{
    PyObject *pChid;
    if(!PyArg_ParseTuple(args, "O", &pChid))
        return NULL;

    chanId chid = (chanId) CAPSULE_EXTRACT(pChid, "chid");
    if (chid == NULL)
        return NULL;

    chtype field_type;
    Py_BEGIN_ALLOW_THREADS
    field_type = ca_field_type(chid);
    Py_END_ALLOW_THREADS

    return Py_BuildValue("l", field_type);
}

static PyObject *Py_ca_element_count(PyObject *self, PyObject *args)
{
    PyObject *pChid;
    if(!PyArg_ParseTuple(args, "O", &pChid))
        return NULL;

    chanId chid = (chanId) CAPSULE_EXTRACT(pChid, "chid");
    if (chid == NULL)
        return NULL;

    unsigned long element_count;
    Py_BEGIN_ALLOW_THREADS
    element_count = ca_element_count(chid);
    Py_END_ALLOW_THREADS

    return Py_BuildValue("k", element_count);
}

static PyObject *Py_ca_name(PyObject *self, PyObject *args)
{
    PyObject *pChid;
    if(!PyArg_ParseTuple(args, "O", &pChid))
        return NULL;

    chanId chid = (chanId) CAPSULE_EXTRACT(pChid, "chid");
    if (chid == NULL)
        return NULL;

    const char *name;
    Py_BEGIN_ALLOW_THREADS
    name = ca_name(chid);
    Py_END_ALLOW_THREADS

    return PyString_FromString(name);
}

static PyObject *Py_ca_state(PyObject *self, PyObject *args)
{
    PyObject *pChid;
    if(!PyArg_ParseTuple(args, "O", &pChid))
        return NULL;

    chanId chid = (chanId) CAPSULE_EXTRACT(pChid, "chid");
    if (chid == NULL)
        return NULL;

    int state;
    Py_BEGIN_ALLOW_THREADS
    state = ca_state(chid); 
    Py_END_ALLOW_THREADS

    return Py_BuildValue("i", state);
}
static PyObject *Py_ca_host_name(PyObject *self, PyObject *args)
{
    PyObject *pChid;
    if(!PyArg_ParseTuple(args, "O", &pChid))
        return NULL;

    chanId chid = (chanId) CAPSULE_EXTRACT(pChid, "chid");
    if (chid == NULL)
        return NULL;

    const char *host;
    Py_BEGIN_ALLOW_THREADS
    host = ca_host_name(chid);
    Py_END_ALLOW_THREADS

    return PyString_FromString(host);
}

static PyObject *Py_ca_read_access(PyObject *self, PyObject *args)
{
    PyObject *pChid;
    if(!PyArg_ParseTuple(args, "O", &pChid))
        return NULL;

    chanId chid = (chanId) CAPSULE_EXTRACT(pChid, "chid");
    if (chid == NULL)
        return NULL;

    int access;
    Py_BEGIN_ALLOW_THREADS
    access = ca_read_access(chid);
    Py_END_ALLOW_THREADS

    return Py_BuildValue("i", access);
}

static PyObject *Py_ca_write_access(PyObject *self, PyObject *args)
{
    PyObject *pChid;
    if(!PyArg_ParseTuple(args, "O", &pChid))
        return NULL;

    chanId chid = (chanId) CAPSULE_EXTRACT(pChid, "chid");
    if (chid == NULL)
        return NULL;
    
    int access;
    Py_BEGIN_ALLOW_THREADS
    access = ca_write_access(chid);
    Py_END_ALLOW_THREADS

    return Py_BuildValue("i", access);
}


/*******************************************************
 *                    Utility                          *
 *******************************************************/

static PyObject *Py_alarmStatusString(PyObject *self, PyObject *args)
{
    int status;

    if(!PyArg_ParseTuple(args, "i", &status))
         return NULL;

    if (status >= ALARM_NSTATUS) {
        PyErr_SetString(PyExc_IndexError, "Alarm status code out of range");
        return NULL;
    }
    return PyString_FromString(alarmStatusString[status]);
}

static PyObject *Py_alarmSeverityString(PyObject *self, PyObject *args)
{
    int severity;

    if(!PyArg_ParseTuple(args, "i", &severity))
         return NULL;

    if (severity >= ALARM_NSEV) {
        PyErr_SetString(PyExc_IndexError, "Alarm severity code out of range");
        return NULL;
    }

    return PyString_FromString(alarmSeverityString[severity]);
}

static PyObject *Py_dbf_text(PyObject *self, PyObject *args)
{
    chtype field_type;

    if(!PyArg_ParseTuple(args, "l", &field_type))
        return NULL;

    return PyString_FromString(dbf_type_to_text(field_type));
}

static PyObject *Py_dbf_text_to_type(PyObject *self, PyObject *args)
{
    char *text = NULL;
    int field_type;

    if(!PyArg_ParseTuple(args, "z", &text))
        return NULL;

    dbf_text_to_type(text, field_type);
    return PyInt_FromLong(field_type);
}

static PyObject *Py_dbr_text(PyObject *self, PyObject *args)
{
    chtype req_type;

    if(!PyArg_ParseTuple(args, "l", &req_type))
         return NULL;

    return PyString_FromString(dbr_type_to_text(req_type));
}

static PyObject *Py_dbr_text_to_type(PyObject *self, PyObject *args)
{
    char *text = NULL;
    int req_type;

    if(!PyArg_ParseTuple(args, "z", &text))
        return NULL;

    dbr_text_to_type(text, req_type);
    return PyInt_FromLong(req_type);
}

static PyObject *Py_ca_message(PyObject *self, PyObject *args)
{
    int status;

    if(!PyArg_ParseTuple(args, "i", &status))
         return NULL;

    return PyString_FromString(ca_message(status));
}

static PyObject *Py_dbf_type_is_valid(PyObject *self, PyObject *args)
{
    int dbftype;

    if(!PyArg_ParseTuple(args, "i", &dbftype))
         return NULL;

    return PyInt_FromLong(dbf_type_is_valid(dbftype));
}

static PyObject *Py_dbf_type_to_DBR(PyObject *self, PyObject *args)
{
    int field_type;

    if(!PyArg_ParseTuple(args, "i", &field_type))
         return NULL;

    return PyInt_FromLong(dbf_type_to_DBR(field_type));
}

static PyObject *Py_dbf_type_to_DBR_STS(PyObject *self, PyObject *args)
{
    int field_type;

    if(!PyArg_ParseTuple(args, "i", &field_type))
         return NULL;

    return PyInt_FromLong(dbf_type_to_DBR_STS(field_type));
}

static PyObject *Py_dbf_type_to_DBR_TIME(PyObject *self, PyObject *args)
{
    int field_type;

    if(!PyArg_ParseTuple(args, "i", &field_type))
         return NULL;

    return PyInt_FromLong(dbf_type_to_DBR_TIME(field_type));
}

static PyObject *Py_dbf_type_to_DBR_GR(PyObject *self, PyObject *args)
{
    int field_type;

    if(!PyArg_ParseTuple(args, "i", &field_type))
         return NULL;

    return PyInt_FromLong(dbf_type_to_DBR_GR(field_type));
}

static PyObject *Py_dbf_type_to_DBR_CTRL(PyObject *self, PyObject *args)
{
    int field_type;

    if(!PyArg_ParseTuple(args, "i", &field_type))
         return NULL;

    return PyInt_FromLong(dbf_type_to_DBR_CTRL(field_type));
}

static PyObject *Py_dbr_type_is_valid(PyObject *self, PyObject *args)
{
    int dbrtype;

    if(!PyArg_ParseTuple(args, "i", &dbrtype))
         return NULL;

    return PyInt_FromLong(dbr_type_is_valid(dbrtype));
}

static PyObject *Py_dbr_type_is_plain(PyObject *self, PyObject *args)
{
    int dbrtype;

    if(!PyArg_ParseTuple(args, "i", &dbrtype))
         return NULL;

    return PyInt_FromLong(dbr_type_is_plain(dbrtype));
}

static PyObject *Py_dbr_type_is_STS(PyObject *self, PyObject *args)
{
    int dbrtype;

    if(!PyArg_ParseTuple(args, "i", &dbrtype))
         return NULL;

    return PyInt_FromLong(dbr_type_is_STS(dbrtype));
}

static PyObject *Py_dbr_type_is_TIME(PyObject *self, PyObject *args)
{
    int dbrtype;

    if(!PyArg_ParseTuple(args, "i", &dbrtype))
         return NULL;

    return PyInt_FromLong(dbr_type_is_TIME(dbrtype));
}

static PyObject *Py_dbr_type_is_GR(PyObject *self, PyObject *args)
{
    int dbrtype;

    if(!PyArg_ParseTuple(args, "i", &dbrtype))
         return NULL;

    return PyInt_FromLong(dbr_type_is_GR(dbrtype));
}

static PyObject *Py_dbr_type_is_CTRL(PyObject *self, PyObject *args)
{
    int dbrtype;

    if(!PyArg_ParseTuple(args, "i", &dbrtype))
         return NULL;

    return PyInt_FromLong(dbr_type_is_CTRL(dbrtype));
}

static PyObject *Py_dbr_type_is_STRING(PyObject *self, PyObject *args)
{
    int dbrtype;

    if(!PyArg_ParseTuple(args, "i", &dbrtype))
         return NULL;

    return PyInt_FromLong(dbr_type_is_STRING(dbrtype));
}

static PyObject *Py_dbr_type_is_SHORT(PyObject *self, PyObject *args)
{
    int dbrtype;

    if(!PyArg_ParseTuple(args, "i", &dbrtype))
         return NULL;

    return PyInt_FromLong(dbr_type_is_SHORT(dbrtype));
}

static PyObject *Py_dbr_type_is_FLOAT(PyObject *self, PyObject *args)
{
    int dbrtype;

    if(!PyArg_ParseTuple(args, "i", &dbrtype))
         return NULL;

    return PyInt_FromLong(dbr_type_is_FLOAT(dbrtype));
}

static PyObject *Py_dbr_type_is_ENUM(PyObject *self, PyObject *args)
{
    int dbrtype;

    if(!PyArg_ParseTuple(args, "i", &dbrtype))
         return NULL;

    return PyInt_FromLong(dbr_type_is_ENUM(dbrtype));
}

static PyObject *Py_dbr_type_is_CHAR(PyObject *self, PyObject *args)
{
    int dbrtype;

    if(!PyArg_ParseTuple(args, "i", &dbrtype))
         return NULL;

    return PyInt_FromLong(dbr_type_is_CHAR(dbrtype));
}

static PyObject *Py_dbr_type_is_LONG(PyObject *self, PyObject *args)
{
    int dbrtype;

    if(!PyArg_ParseTuple(args, "i", &dbrtype))
         return NULL;

    return PyInt_FromLong(dbr_type_is_LONG(dbrtype));
}

static PyObject *Py_dbr_type_is_DOUBLE(PyObject *self, PyObject *args)
{
    int dbrtype;

    if(!PyArg_ParseTuple(args, "i", &dbrtype))
         return NULL;

    return PyInt_FromLong(dbr_type_is_DOUBLE(dbrtype));
}

/*******************************************************
 *                DBR value conversion routine         *
 *******************************************************/
/*
    Convert from C dbr value to Python dictionary
*/

typedef dbr_string_t *dbr_string_t_ptr;
typedef dbr_char_t *dbr_char_t_ptr ;

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
            memcpy(PyArray_DATA((PyArrayObject*)value), VAL, COUNT*sizeof(PYTYPE)); \
        }\
    }
#define FormatDBRtoValueArray(COUNT, VP, PYTYPE, FORMAT,  NPTYPE) \
    {\
        if(COUNT==1)\
            value = FORMAT((PYTYPE) cval->value);\
        else{\
            npy_intp ndims[]  = {COUNT};\
            value =  PyArray_SimpleNew(1, ndims, NPTYPE); \
            memcpy(PyArray_DATA((PyArrayObject*)value), VP, COUNT*sizeof(PYTYPE)); \
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

#define TS2secs(ts) ((double)ts.secPastEpoch + POSIX_TIME_AT_EPICS_EPOCH + (double) ts.nsec*1.0e-9)

PyObject * CBufferToPythonDict(chtype type,
                    unsigned long count,
                    const void *val,
                    int use_numpy)
{
    PyObject *arglist = NULL;
    PyObject *value   = NULL;

    /* build arglist, value is inserted */
    switch(type){
    case DBR_STRING:
        FormatPlaintoValue(count, val, dbr_string_t, PyString_FromString);
        arglist = Py_BuildValue("O", value);
        break;
    case DBR_SHORT:
        if(use_numpy == 0 || has_numpy == 0)
            FormatPlaintoValue(count, val, dbr_short_t,  PyInt_FromLong)
        #ifdef WITH_NUMPY
        else
            FormatPlaintoValueArray(count, val, dbr_short_t,  PyInt_FromLong,  NPY_SHORT)
        #endif
        arglist = Py_BuildValue("O", value);
        break;
    case DBR_FLOAT:
        if(use_numpy == 0 || has_numpy == 0)
            FormatPlaintoValue(count, val, dbr_float_t,  PyFloat_FromDouble)
        #ifdef WITH_NUMPY
        else
            FormatPlaintoValueArray(count, val, dbr_float_t,  PyFloat_FromDouble,  NPY_FLOAT)
        #endif
        arglist = Py_BuildValue("O", value);
        break;
    case DBR_ENUM:
        if(use_numpy == 0 || has_numpy == 0)
            FormatPlaintoValue(count, val, dbr_enum_t,   PyInt_FromLong)
        #ifdef WITH_NUMPY
        else
            FormatPlaintoValueArray(count, val, dbr_enum_t,  PyInt_FromLong,  NPY_USHORT)
        #endif
        arglist = Py_BuildValue("O", value);
        break;
    case DBR_CHAR:
        if (use_numpy == 0 || has_numpy == 0)
            FormatPlaintoValue(count, val, dbr_char_t,   PyInt_FromLong)
        #ifdef WITH_NUMPY
        else
            FormatPlaintoValueArray(count, val, dbr_char_t,   PyInt_FromLong,  NPY_BYTE)
        #endif
        arglist = Py_BuildValue("O", value);
        break;
    case DBR_LONG:
        if (use_numpy == 0 || has_numpy == 0)
            FormatPlaintoValue(count, val, dbr_long_t,   PyInt_FromLong)
        #ifdef WITH_NUMPY
        else
            FormatPlaintoValueArray(count, val, dbr_long_t,   PyInt_FromLong,  NPY_INT)
        #endif
        arglist = Py_BuildValue("O", value);
        break;
    case DBR_DOUBLE:
        if (use_numpy == 0 || has_numpy == 0)
            FormatPlaintoValue(count, val, dbr_double_t, PyFloat_FromDouble)
        #ifdef WITH_NUMPY
        else
            FormatPlaintoValueArray(count, val, dbr_double_t, PyFloat_FromDouble, NPY_DOUBLE)
        #endif
        arglist = Py_BuildValue("O", value);
        break;

    case DBR_STS_STRING:
    case DBR_GR_STRING:
    case DBR_CTRL_STRING:
    {
        struct dbr_sts_string  *cval=(struct dbr_sts_string  *)val;
        FormatDBRStringtoValue(count, string,  cval->value , dbr_string_t_ptr, PyString_FromString);
        arglist=Py_BuildValue("{s:O,s:i,s:i}",
                "value",    value,
                "severity", cval->severity,
                "status",   cval->status);
    }
        break;
    case DBR_STS_SHORT:
    {
        struct dbr_sts_short  *cval=(struct dbr_sts_short  *)val;
        if (use_numpy == 0 || has_numpy == 0)
            FormatDBRtoValue(count, short, &(cval->value), dbr_short_t,   PyInt_FromLong)
        #ifdef WITH_NUMPY
        else
            FormatDBRtoValueArray(count, &(cval->value), dbr_short_t,   PyInt_FromLong, NPY_SHORT)
        #endif
        arglist=Py_BuildValue("{s:O,s:i,s:i}",
                "value",    value,
                "severity", cval->severity,
                "status",   cval->status);
    }
        break;
    case DBR_STS_FLOAT:
    {
        struct dbr_sts_float  *cval=(struct dbr_sts_float  *)val;
        if (use_numpy == 0 || has_numpy == 0)
            FormatDBRtoValue(count, float, &(cval->value), dbr_float_t, PyFloat_FromDouble)
        #ifdef WITH_NUMPY
        else
            FormatDBRtoValueArray(count, &(cval->value), dbr_float_t, PyFloat_FromDouble, NPY_FLOAT)
        #endif
        arglist=Py_BuildValue("{s:O,s:i,s:i}",
                "value",    value,
                "severity", cval->severity,
                "status",   cval->status);
    }
        break;
    case DBR_STS_ENUM:
    {
        struct dbr_sts_enum  *cval=(struct dbr_sts_enum  *)val;
        if (use_numpy == 0 || has_numpy == 0)
            FormatDBRtoValue(count, enum, &(cval->value),  dbr_enum_t,     PyInt_FromLong)
        #ifdef WITH_NUMPY
        else
            FormatDBRtoValueArray(count,  &(cval->value), dbr_enum_t,      PyInt_FromLong, NPY_USHORT)
        #endif
        arglist=Py_BuildValue("{s:O,s:i,s:i}",
                "value",    value,
                "severity", cval->severity,
                "status",   cval->status);
    }
        break;
    case DBR_STS_CHAR:
    {
        struct dbr_sts_char  *cval=(struct dbr_sts_char  *)val;
        if (use_numpy == 0 || has_numpy == 0)
            FormatDBRtoValue(count, char, &(cval->value), dbr_char_t,     PyInt_FromLong)
        #ifdef WITH_NUMPY
        else
            FormatDBRtoValueArray(count,  &(cval->value), dbr_char_t,     PyInt_FromLong, NPY_BYTE)
        #endif
        arglist=Py_BuildValue("{s:O,s:i,s:i}",
                "value",    value,
                "severity", cval->severity,
                "status",   cval->status);
    }
        break;
    case DBR_STS_LONG:
    {
        struct dbr_sts_long  *cval=(struct dbr_sts_long  *)val;
        if (use_numpy == 0 || has_numpy == 0)
            FormatDBRtoValue(count, long, &(cval->value), dbr_long_t,     PyInt_FromLong)
        #ifdef WITH_NUMPY
        else
            FormatDBRtoValueArray(count,  &(cval->value), dbr_long_t,     PyInt_FromLong, NPY_INT)
        #endif
        arglist=Py_BuildValue("{s:O,s:i,s:i}",
                "value",    value,
                "severity", cval->severity,
                "status",   cval->status);
    }
        break;
    case DBR_STS_DOUBLE:
    {
        struct dbr_sts_double  *cval=(struct dbr_sts_double  *)val;
        if (use_numpy == 0 || has_numpy == 0)
            FormatDBRtoValue(count, double, &(cval->value), dbr_double_t, PyFloat_FromDouble)
        #ifdef WITH_NUMPY
        else
            FormatDBRtoValueArray(count,    &(cval->value), dbr_double_t, PyFloat_FromDouble, NPY_DOUBLE)
        #endif
        arglist=Py_BuildValue("{s:O,s:i,s:i}",
                "value",    value,
                "severity", cval->severity,
                "status",   cval->status);
    }
        break;

    case DBR_TIME_STRING:
    {
        struct dbr_time_string  *cval=(struct dbr_time_string  *)val;
        FormatDBRStringtoValue(count, string,  cval->value, dbr_string_t_ptr, PyString_FromString);
        arglist=Py_BuildValue("{s:O,s:i,s:i,s:d}",
                "value",    value,
                "severity", cval->severity,
                "status",   cval->status,
                "stamp",    TS2secs(cval->stamp));
    }
        break;
    case DBR_TIME_SHORT:
    {
        struct dbr_time_short  *cval=(struct dbr_time_short  *)val;
        if (use_numpy == 0 || has_numpy == 0)
            FormatDBRtoValue(count, short, &(cval->value), dbr_short_t,      PyInt_FromLong)
        #ifdef WITH_NUMPY
        else
            FormatDBRtoValueArray(count,   &(cval->value), dbr_short_t,      PyInt_FromLong, NPY_SHORT)
        #endif
        arglist=Py_BuildValue("{s:O,s:i,s:i,s:d}",
                "value",    value,
                "severity", cval->severity,
                "status",   cval->status,
                "stamp",    TS2secs(cval->stamp));
    }
        break;
    case DBR_TIME_FLOAT:
    {
        struct dbr_time_float  *cval=(struct dbr_time_float  *)val;
        if (use_numpy == 0 || has_numpy == 0)
            FormatDBRtoValue(count, float, &(cval->value), dbr_float_t,      PyFloat_FromDouble)
        #ifdef WITH_NUMPY
        else
            FormatDBRtoValueArray(count,   &(cval->value), dbr_float_t,      PyFloat_FromDouble, NPY_FLOAT)
        #endif
        arglist=Py_BuildValue("{s:O,s:i,s:i,s:d}",
                "value",    value,
                "severity", cval->severity,
                "status",   cval->status,
                "stamp",    TS2secs(cval->stamp));
    }
        break;
    case DBR_TIME_ENUM:
    {
        struct dbr_time_enum  *cval=(struct dbr_time_enum  *)val;
        if (use_numpy == 0 || has_numpy == 0)
            FormatDBRtoValue(count, enum, &(cval->value), dbr_enum_t,        PyInt_FromLong)
        #ifdef WITH_NUMPY
        else
            FormatDBRtoValueArray(count,  &(cval->value), dbr_enum_t,        PyInt_FromLong, NPY_USHORT)
        #endif
        arglist=Py_BuildValue("{s:O,s:i,s:i,s:d}",
                "value",    value,
                "severity", cval->severity,
                "status",   cval->status,
                "stamp",    TS2secs(cval->stamp));
    }
        break;
    case DBR_TIME_CHAR:
    {
        struct dbr_time_char  *cval=(struct dbr_time_char  *)val;
        if (use_numpy == 0 || has_numpy == 0)
            FormatDBRtoValue(count, char, &(cval->value), dbr_char_t,        PyInt_FromLong)
        #ifdef WITH_NUMPY
        else
            FormatDBRtoValueArray(count,  &(cval->value), dbr_char_t,        PyInt_FromLong, NPY_BYTE)
        #endif
        arglist=Py_BuildValue("{s:O,s:i,s:i,s:d}",
                "value",    value,
                "severity", cval->severity,
                "status",   cval->status,
                "stamp",    TS2secs(cval->stamp));
    }
        break;
    case DBR_TIME_LONG:
    {
        struct dbr_time_long  *cval=(struct dbr_time_long  *)val;
        if (use_numpy == 0  || has_numpy == 0)
            FormatDBRtoValue(count, long, &(cval->value), dbr_long_t,        PyInt_FromLong)
        #ifdef WITH_NUMPY
        else
            FormatDBRtoValueArray(count,  &(cval->value), dbr_long_t,        PyInt_FromLong, NPY_INT)
        #endif
        arglist=Py_BuildValue("{s:O,s:i,s:i,s:d}",
                "value",    value,
                "severity", cval->severity,
                "status",   cval->status,
                "stamp",    TS2secs(cval->stamp));
    }
        break;
    case DBR_TIME_DOUBLE:
    {
        struct dbr_time_double  *cval=(struct dbr_time_double  *)val;
        if (use_numpy == 0  || has_numpy == 0)
            FormatDBRtoValue(count, double, &(cval->value), dbr_double_t,    PyFloat_FromDouble)
        #ifdef WITH_NUMPY
        else
            FormatDBRtoValueArray(count,    &(cval->value), dbr_double_t,    PyFloat_FromDouble, NPY_DOUBLE)
        #endif
        arglist=Py_BuildValue("{s:O,s:i,s:i,s:d}",
                "value",    value,
                "severity", cval->severity,
                "status",   cval->status,
                "stamp",    TS2secs(cval->stamp));
    }
        break;

    case DBR_GR_SHORT:
    {
        struct dbr_gr_short  *cval=(struct dbr_gr_short  *)val;
        if (use_numpy == 0  || has_numpy == 0)
            FormatDBRtoValue(count, short,  &(cval->value), dbr_short_t,     PyInt_FromLong)
        #ifdef WITH_NUMPY
        else
             FormatDBRtoValueArray(count,   &(cval->value), dbr_short_t,     PyInt_FromLong, NPY_SHORT)
        #endif
        arglist=Py_BuildValue("{s:O,s:i,s:i,s:s,s:i,s:i,s:i,s:i,s:i,s:i}",
                "value",    value,
                "severity", cval->severity,
                "status",   cval->status,
                "units",    cval->units,
                "upper_disp_limit",     cval->upper_disp_limit,
                "lower_disp_limit",     cval->lower_disp_limit,
                "upper_alarm_limit",    cval->upper_alarm_limit,
                "upper_warning_limit",  cval->upper_warning_limit,
                "lower_alarm_limit",    cval->lower_alarm_limit,
                "lower_warning_limit",  cval->lower_warning_limit
                );
    }
        break;
    case DBR_GR_FLOAT:
    {
        struct dbr_gr_float  *cval=(struct dbr_gr_float  *)val;
        if (use_numpy == 0  || has_numpy == 0)
            FormatDBRtoValue(count, float,  &(cval->value), dbr_float_t,     PyFloat_FromDouble)
        #ifdef WITH_NUMPY
        else
            FormatDBRtoValueArray(count,    &(cval->value), dbr_float_t,     PyFloat_FromDouble, NPY_FLOAT)
        #endif
        arglist=Py_BuildValue("{s:O,s:i,s:i,s:s,s:f,s:f,s:f,s:f,s:f,s:f,s:i}",
                "value",    value,
                "severity", cval->severity,
                "status",   cval->status,
                "units",    cval->units,
                "upper_disp_limit",     cval->upper_disp_limit,
                "lower_disp_limit",     cval->lower_disp_limit,
                "upper_alarm_limit",    cval->upper_alarm_limit,
                "upper_warning_limit",  cval->upper_warning_limit,
                "lower_alarm_limit",    cval->lower_alarm_limit,
                "lower_warning_limit",  cval->lower_warning_limit,
                "precision",            cval->precision
                );
    }
        break;
    case DBR_GR_ENUM:
    {
        struct dbr_gr_enum  *cval=(struct dbr_gr_enum  *)val;
        if (use_numpy == 0 || has_numpy == 0)
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
        arglist=Py_BuildValue("{s:O,s:i,s:i,s:i,s:O}",
                "value",    value,
                "severity", cval->severity,
                "status",   cval->status,
                "no_str",   cval->no_str,
                "strs",     ptup
                );
        Py_XDECREF(ptup);
    }
        break;
    case DBR_GR_CHAR:
    {
        struct dbr_gr_char  *cval=(struct dbr_gr_char  *)val;
        if (use_numpy == 0 || has_numpy == 0)
            FormatDBRtoValue(count, char,  &(cval->value), dbr_char_t,        PyInt_FromLong)
        #ifdef WITH_NUMPY
        else
            FormatDBRtoValueArray(count,   &(cval->value), dbr_char_t,        PyInt_FromLong, NPY_BYTE)
        #endif
        arglist=Py_BuildValue("{s:O,s:i,s:i,s:s,s:b,s:b,s:b,s:b,s:b,s:b}",
                "value",    value,
                "severity", cval->severity,
                "status",   cval->status,
                "units",    cval->units,
                "upper_disp_limit",     cval->upper_disp_limit,
                "lower_disp_limit",     cval->lower_disp_limit,
                "upper_alarm_limit",    cval->upper_alarm_limit,
                "upper_warning_limit",  cval->upper_warning_limit,
                "lower_alarm_limit",    cval->lower_alarm_limit,
                "lower_warning_limit",  cval->lower_warning_limit
                );
    }
        break;
    case DBR_GR_LONG:
    {
        struct dbr_gr_long  *cval=(struct dbr_gr_long  *)val;
        if (use_numpy == 0 || has_numpy == 0)
            FormatDBRtoValue(count, long,  &(cval->value), dbr_long_t,       PyInt_FromLong)
        #ifdef WITH_NUMPY
        else
            FormatDBRtoValueArray(count,   &(cval->value), dbr_long_t,       PyInt_FromLong, NPY_INT)
        #endif
        arglist=Py_BuildValue("{s:O,s:i,s:i,s:s,s:i,s:i,s:i,s:i,s:i,s:i}",
                "value",    value,
                "severity", cval->severity,
                "status",   cval->status,
                "units",    cval->units,
                "upper_disp_limit",     cval->upper_disp_limit,
                "lower_disp_limit",     cval->lower_disp_limit,
                "upper_alarm_limit",    cval->upper_alarm_limit,
                "upper_warning_limit",  cval->upper_warning_limit,
                "lower_alarm_limit",    cval->lower_alarm_limit,
                "lower_warning_limit",  cval->lower_warning_limit
                );
    }
        break;

    case DBR_GR_DOUBLE:
    {
        struct dbr_gr_double  *cval=(struct dbr_gr_double  *)val;
        if (use_numpy == 0 || has_numpy == 0)
            FormatDBRtoValue(count, double,  &(cval->value), dbr_double_t,    PyFloat_FromDouble)
        #ifdef WITH_NUMPY
        else
            FormatDBRtoValueArray(count,     &(cval->value), dbr_double_t,    PyFloat_FromDouble, NPY_DOUBLE)
        #endif
        arglist=Py_BuildValue("{s:O,s:i,s:i,s:s,s:d,s:d,s:d,s:d,s:d,s:d,s:i}",
                "value",    value,
                "severity", cval->severity,
                "status",   cval->status,
                "units",    cval->units,
                "upper_disp_limit",     cval->upper_disp_limit,
                "lower_disp_limit",     cval->lower_disp_limit,
                "upper_alarm_limit",    cval->upper_alarm_limit,
                "upper_warning_limit",  cval->upper_warning_limit,
                "lower_alarm_limit",    cval->lower_alarm_limit,
                "lower_warning_limit",  cval->lower_warning_limit,
                "precision",            cval->precision
                );
    }
        break;

    case DBR_CTRL_SHORT:
    {
        struct dbr_ctrl_short  *cval=(struct dbr_ctrl_short  *)val;
        if (use_numpy == 0 || has_numpy == 0)
            FormatDBRtoValue(count, short,  &(cval->value), dbr_short_t,      PyInt_FromLong)
        #ifdef WITH_NUMPY
        else
            FormatDBRtoValueArray(count,    &(cval->value), dbr_short_t,      PyInt_FromLong, NPY_SHORT)
        #endif
        arglist=Py_BuildValue("{s:O,s:i,s:i,s:s,s:i,s:i,s:i,s:i,s:i,s:i,s:i,s:i}",
                "value",    value,
                "severity", cval->severity,
                "status",   cval->status,
                "units",    cval->units,
                "upper_disp_limit",     cval->upper_disp_limit,
                "lower_disp_limit",     cval->lower_disp_limit,
                "upper_alarm_limit",    cval->upper_alarm_limit,
                "upper_warning_limit",  cval->upper_warning_limit,
                "lower_alarm_limit",    cval->lower_alarm_limit,
                "lower_warning_limit",  cval->lower_warning_limit,
                "upper_ctrl_limit",     cval->upper_ctrl_limit,
                "lower_ctrl_limit",     cval->lower_ctrl_limit
                );
    }
        break;
    case DBR_CTRL_FLOAT:
    {
        struct dbr_ctrl_float  *cval=(struct dbr_ctrl_float  *)val;
        if (use_numpy == 0 || has_numpy == 0)
            FormatDBRtoValue(count, float,  &(cval->value), dbr_float_t,      PyFloat_FromDouble)
        #ifdef WITH_NUMPY
        else
             FormatDBRtoValueArray(count,   &(cval->value), dbr_float_t,      PyFloat_FromDouble, NPY_FLOAT)
        #endif
        arglist=Py_BuildValue("{s:O,s:i,s:i,s:s,s:f,s:f,s:f,s:f,s:f,s:f,s:f,s:f,s:i}",
                "value",    value,
                "severity", cval->severity,
                "status",   cval->status,
                "units",    cval->units,
                "upper_disp_limit",     cval->upper_disp_limit,
                "lower_disp_limit",     cval->lower_disp_limit,
                "upper_alarm_limit",    cval->upper_alarm_limit,
                "upper_warning_limit",  cval->upper_warning_limit,
                "lower_alarm_limit",    cval->lower_alarm_limit,
                "lower_warning_limit",  cval->lower_warning_limit,
                "upper_ctrl_limit",     cval->upper_ctrl_limit,
                "lower_ctrl_limit",     cval->lower_ctrl_limit,
                "precision",            cval->precision
                );
    }
        break;
    case DBR_CTRL_ENUM:
    {
        struct dbr_ctrl_enum  *cval=(struct dbr_ctrl_enum  *)val;
        if (use_numpy == 0 || has_numpy == 0)
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
        arglist=Py_BuildValue("{s:O,s:i,s:i,s:i,s:O}",
                "value",    value,
                "severity", cval->severity,
                "status",   cval->status,
                "no_str",   cval->no_str,
                "strs",     ptup
                );
        Py_XDECREF(ptup);
    }
        break;
    case DBR_CTRL_CHAR:
    {
        struct dbr_ctrl_char  *cval=(struct dbr_ctrl_char  *)val;
        if (use_numpy == 0 || has_numpy == 0)
            FormatDBRtoValue(count, char,  &(cval->value), dbr_char_t,        PyInt_FromLong)
        #ifdef WITH_NUMPY
        else
            FormatDBRtoValueArray(count,   &(cval->value), dbr_char_t,        PyInt_FromLong, NPY_BYTE)
        #endif
        arglist=Py_BuildValue("{s:O,s:i,s:i,s:s,s:b,s:b,s:b,s:b,s:b,s:b,s:b,s:b}",
                "value",    value,
                "severity", cval->severity,
                "status",   cval->status,
                "units",    cval->units,
                "upper_disp_limit",     cval->upper_disp_limit,
                "lower_disp_limit",     cval->lower_disp_limit,
                "upper_alarm_limit",    cval->upper_alarm_limit,
                "upper_warning_limit",  cval->upper_warning_limit,
                "lower_alarm_limit",    cval->lower_alarm_limit,
                "lower_warning_limit",  cval->lower_warning_limit,
                "upper_ctrl_limit",     cval->upper_ctrl_limit,
                "lower_ctrl_limit",     cval->lower_ctrl_limit
                );
    }
        break;
    case DBR_CTRL_LONG:
    {
        struct dbr_ctrl_long  *cval=(struct dbr_ctrl_long  *)val;
        if (use_numpy == 0 || has_numpy == 0)
            FormatDBRtoValue(count, long,  &(cval->value), dbr_long_t,        PyInt_FromLong)
        #ifdef WITH_NUMPY
        else
            FormatDBRtoValueArray(count,   &(cval->value), dbr_long_t,        PyInt_FromLong, NPY_INT)
        #endif
        arglist=Py_BuildValue("{s:O,s:i,s:i,s:s,s:i,s:i,s:i,s:i,s:i,s:i,s:i,s:i}",
                "value",    value,
                "severity", cval->severity,
                "status",   cval->status,
                "units",    cval->units,
                "upper_disp_limit",     cval->upper_disp_limit,
                "lower_disp_limit",     cval->lower_disp_limit,
                "upper_alarm_limit",    cval->upper_alarm_limit,
                "upper_warning_limit",  cval->upper_warning_limit,
                "lower_alarm_limit",    cval->lower_alarm_limit,
                "lower_warning_limit",  cval->lower_warning_limit,
                "upper_ctrl_limit",     cval->upper_ctrl_limit,
                "lower_ctrl_limit",     cval->lower_ctrl_limit
                );
    }
        break;

    case DBR_CTRL_DOUBLE:
    {
        struct dbr_ctrl_double  *cval=(struct dbr_ctrl_double  *)val;
        if (use_numpy == 0 || has_numpy == 0)
            FormatDBRtoValue(count, double,  &(cval->value), dbr_double_t,    PyFloat_FromDouble)
        #ifdef WITH_NUMPY
        else
            FormatDBRtoValueArray(count,     &(cval->value), dbr_double_t,    PyFloat_FromDouble, NPY_DOUBLE)
        #endif
        arglist=Py_BuildValue("{s:O,s:i,s:i,s:s,s:d,s:d,s:d,s:d,s:d,s:d,s:d,s:d,s:i}",
                "value",    value,
                "severity", cval->severity,
                "status",   cval->status,
                "units",    cval->units,
                "upper_disp_limit",     cval->upper_disp_limit,
                "lower_disp_limit",     cval->lower_disp_limit,
                "upper_alarm_limit",    cval->upper_alarm_limit,
                "upper_warning_limit",  cval->upper_warning_limit,
                "lower_alarm_limit",    cval->lower_alarm_limit,
                "lower_warning_limit",  cval->lower_warning_limit,
                "upper_ctrl_limit",     cval->upper_ctrl_limit,
                "lower_ctrl_limit",     cval->lower_ctrl_limit,
                "precision",            cval->precision
                );
    }
        break;
    case DBR_STSACK_STRING:
    {
        struct dbr_stsack_string  *cval=(struct dbr_stsack_string *)val;
        arglist = Py_BuildValue("{s:i,s:i,s:i,s:i,s:s}",
                "status",   cval->status,
                "severity", cval->severity,
                "ackt",     cval->ackt,
                "acks",     cval->acks,
                "value",    cval->value
                );
    }
    break;
    case DBR_CLASS_NAME:
    {
        FormatPlaintoValue(count, val, dbr_string_t,  PyString_FromString)
        arglist = Py_BuildValue("O", value);
    }
    break;
    }
    Py_XDECREF(value);
    return arglist;
}

#define PythonValueToCBuffer(DBRTYPE, COUNT, FORMAT) \
      {\
        DBRTYPE *ptr = (DBRTYPE *) pbuf;\
        if (count == 1) \
            PyArg_Parse(pValue, FORMAT, ptr);\
        else {\
            for(unsigned long i=0; i<count; i++) {\
                PyObject *item = PySequence_GetItem(pValue, i);\
                PyArg_Parse(item, FORMAT, ptr+i);\
                Py_XDECREF(item);\
            }\
        }\
      }

void *setup_put(chanId chid, PyObject *pValue, PyObject *pType, PyObject *pCount,
                                chtype &dbrtype, unsigned long &count)
{
    void *pbuf = NULL;
    chtype field_type = -1;
    unsigned long element_count = 0;

    Py_BEGIN_ALLOW_THREADS
    field_type = ca_field_type(chid);
    element_count = ca_element_count(chid);
    Py_END_ALLOW_THREADS

    if (pType == Py_None)
        dbrtype = dbf_type_to_DBR(field_type);
    else
        dbrtype = PyLong_AsLong(pType);

    // incr refcnt and we will decr at the end
    Py_XINCREF(pValue);

    // sequence object (including string/bytes)
    if (PySequence_Check(pValue)) {
        unsigned long value_count = (unsigned long)PySequence_Length(pValue);

        if (PyUnicode_Check(pValue) || PyBytes_Check(pValue)) {
            // string can be written to enum or string type and the count is 1
            if (dbrtype == DBR_ENUM || dbrtype == DBR_STRING) {
                dbrtype = DBR_STRING;
                value_count = 1;
            } else {
                // for other numeric types, convert string/bytes to list of integers with 0 appended.
                // equivalent to [ord(x) for x in value] + [0]
                char * pBuff = NULL;
                Py_ssize_t buff_size = 0;
                PyArg_Parse(pValue, "z#", &pBuff, &buff_size);

                value_count = (unsigned long)(buff_size + 1);
                PyObject *pCharList = PyList_New(value_count);
                for(int i=0; i<buff_size && pBuff!=NULL; i++) {
                    PyList_SetItem(pCharList, i, PyInt_FromLong(pBuff[i]));
                }
                PyList_SetItem(pCharList, buff_size, PyInt_FromLong(0));
                // the new list replaces string/bytes object
                Py_XDECREF(pValue);
                pValue = pCharList;
            }
        }
        if (pCount != Py_None)
            value_count = MIN(value_count, PyLong_AsUnsignedLong(pCount));

        count = MIN(element_count, value_count);


        if (count == 1 && dbrtype != DBR_STRING) {
            PyObject *item = PySequence_GetItem(pValue, 0);
            Py_XDECREF(pValue);
            pValue = item;
        }
    }

    pbuf = calloc(count, dbr_value_size[dbrtype]);
    switch (dbrtype) {
    case DBR_STRING:
    {
        dbr_string_t *ptr = (dbr_string_t *) pbuf;
        if (count == 1) {
            char *str = NULL;
	        PyArg_Parse(pValue, "z", &str);
	        if (str != NULL)
	            strncpy(ptr[0], str, sizeof(dbr_string_t));
        } else {
            for(unsigned long i=0; i<count; i++) {
                PyObject *item = PySequence_GetItem(pValue, i);
                char *str = NULL;
	            PyArg_Parse(item, "z", &str);
	            if (str != NULL)
	                strncpy(ptr[i], str, sizeof(dbr_string_t));
                Py_XDECREF(item);
            }
        }
    }
    break;
    case DBR_SHORT:
        PythonValueToCBuffer(dbr_short_t,    count, "h");
    break;
    case DBR_FLOAT:
        PythonValueToCBuffer(dbr_float_t,    count, "f");
    break;
    case DBR_CHAR:
        PythonValueToCBuffer(dbr_char_t,     count, "b");
    break;
    case DBR_ENUM:
        PythonValueToCBuffer(dbr_enum_t,     count, "h");
    break;
    case DBR_LONG:
        PythonValueToCBuffer(dbr_long_t,     count, "i");
    break;
    case DBR_DOUBLE:
        PythonValueToCBuffer(dbr_double_t,   count, "d");
    break;
    case DBR_PUT_ACKT:
        PythonValueToCBuffer(dbr_put_ackt_t, count, "h");
    break;
    case DBR_PUT_ACKS:
        PythonValueToCBuffer(dbr_put_acks_t, count, "h");
    break;
    default:
        free(pbuf);
        pbuf = NULL;
    }

    Py_XDECREF(pValue);

    // caller must free the memory
    return pbuf;
}
