
#include <Python.h>

#include <cadef.h>

#ifdef WITH_NUMPY
#include <numpy/arrayobject.h>
int with_numpy = 1;
#else
int  with_numpy = 0;
#endif

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
    #define PyString_AsString PyUnicode_AsString
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

static PyObject *Py_ca_create_channel(PyObject *self, PyObject *args);
static PyObject *Py_ca_clear_channel(PyObject *self, PyObject *args);
static PyObject *Py_ca_get(PyObject *self, PyObject *args);
static PyObject *Py_ca_put(PyObject *self, PyObject *args);
static PyObject *Py_ca_create_subscription(PyObject *self, PyObject *args);
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
static PyObject *Py_ca_sg_get(PyObject *self, PyObject *args);
static PyObject *Py_ca_sg_put(PyObject *self, PyObject *args);
static PyObject *Py_ca_sg_test(PyObject *self, PyObject *args);
static PyObject *Py_ca_sg_reset(PyObject *self, PyObject *args);
static PyObject *Py_ca_sg_block(PyObject *self, PyObject *args);

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

static PyObject *Py_dbf_text(PyObject *self, PyObject *args);
static PyObject *Py_dbr_text(PyObject *self, PyObject *args);
static PyObject *Py_ca_message(PyObject *self, PyObject *args);

static PyObject *CBufferToPythonDict(chtype type, unsigned long count, const void *val, int use_numpy);


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
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
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
    {"create_channel",      Py_ca_create_channel,   METH_VARARGS, "Create a CA channel connection"},
    {"clear_channel",       Py_ca_clear_channel,    METH_VARARGS, "Shutdown a CA channel connection"},
    {"get",                 Py_ca_get,              METH_VARARGS, "Read PV's value"},
    {"put",                 Py_ca_put,              METH_VARARGS, "Write a value to PV"},
    {"create_subscription", Py_ca_create_subscription,METH_VARARGS,"Subscribe for state changes"},
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
    {"sg_get",      Py_ca_sg_get,       METH_VARARGS, "Read PV's value"},
    {"sg_put",      Py_ca_sg_put,       METH_VARARGS, "Write a value to PV"},
    {"sg_reset",    Py_ca_sg_reset,     METH_VARARGS, "Reset outstanding requests"},
    {"sg_block",    Py_ca_sg_block,     METH_VARARGS, "wait pending get"},
    {"sg_test",     Py_ca_sg_test,      METH_VARARGS, "check pending get"},

    /* Utility */
    {"dbf_text",    Py_dbf_text,    METH_VARARGS, "field type name"},
    {"dbr_text",    Py_dbr_text,    METH_VARARGS, "request type name"},
    {"message",     Py_ca_message,  METH_VARARGS, "status error message"},
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
        with_numpy = 0;
        PyErr_Clear();
    }
    #endif

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
    struct ca_client_context *pContext = ca_current_context();

    if (pContext == NULL)
        Py_RETURN_NONE;
    else
        return CAPSULE_BUILD(pContext, "ca_client_context", NULL);
}

static PyObject *Py_ca_show_context(PyObject *self, PyObject *args, PyObject *kws)
{
    PyObject *pObject = NULL;
    int level = 0;

    static char *kwlist[] = {(char*)"context", (char*)"level",  NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kws, "|Oi", kwlist, &pObject, &level))
        return NULL;

    struct ca_client_context *pContext = NULL;
    if(pObject == NULL)
        pContext = ca_current_context();
    else
        pContext = (struct ca_client_context *)CAPSULE_EXTRACT(pObject, "ca_client_context");

    if (pContext != NULL)
        ca_context_status(pContext, level);

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
    ChannelData(PyObject *pCallback, PyObject *pArgs) : pAccessEventCallback(NULL),pAccessEventArgs(NULL) {
        this->pCallback = pCallback;
        this->pArgs = pArgs;
        Py_XINCREF(pCallback);
        Py_XINCREF(pArgs);
    }
    ~ChannelData() {
        Py_XDECREF(pCallback);
        Py_XDECREF(pArgs);
        Py_XDECREF(pAccessEventCallback);
        Py_XDECREF(pAccessEventArgs);
    }

    PyObject *pCallback;
    PyObject *pArgs;
    evid eventID;
    PyObject *pAccessEventCallback;
    PyObject *pAccessEventArgs;
};

static void connection_callback(struct connection_handler_args args)
{
    PyGILState_STATE gstate = PyGILState_Ensure();

    ChannelData *pData = (ChannelData *) ca_puser(args.chid);

    if(PyCallable_Check(pData->pCallback)) {
        PyObject *pChid = CAPSULE_BUILD(args.chid, "chid", NULL);
        PyObject *pArgs = Py_BuildValue("({s:O,s:i},O)", "chid", pChid, "op", args.op, pData->pArgs);

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

static PyObject *Py_ca_create_channel(PyObject *self, PyObject *args)
{
    char *pName;
    PyObject *pCallback = NULL;
    PyObject *pArgs = NULL;
    int priority = 0;
    if(!PyArg_ParseTuple(args, "z|OOi", &pName, &pCallback, &pArgs, &priority))
        return NULL;

    chanId chid = NULL;
    int status;

    ChannelData *pData = new ChannelData(pCallback, pArgs);
    if(PyCallable_Check(pCallback)) {
        status = ca_create_channel(pName, &connection_callback, pData, priority, &chid);
        if (status != ECA_NORMAL) delete pData;
    } else {
        status = ca_create_channel(pName, NULL, pData, priority, &chid);
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

    ChannelData *pData = (ChannelData *) ca_puser(chid);

    int status;

    Py_BEGIN_ALLOW_THREADS
    status = ca_clear_channel(chid);
    Py_END_ALLOW_THREADS

    delete pData;

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
                    false);
        PyObject *pArgs = Py_BuildValue(
        "({s:O,s:i,s:i,s:i,s:O}, O)",
        "chid", pChid,
        "type", args.type,
        "count", args.count,
        "status", args.status,
        "value", pValue,
        pData->pArgs
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
                    false);
        PyObject *pArgs = Py_BuildValue(
        "({s:O,s:i,s:i,s:i,s:O}, O)",
        "chid", pChid,
        "type", args.type,
        "count", args.count,
        "status", args.status,
        "value", pValue,
        pData->pArgs
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


static PyObject *Py_ca_get(PyObject *self, PyObject *args)
{
    PyObject *pChid;
    chtype dbrtype = -1;
    unsigned long count = 0;
    PyObject *pCallback = NULL;
    PyObject *pArgs = NULL;
    if(!PyArg_ParseTuple(args, "O|lkOO", &pChid, &dbrtype, &count, &pCallback, &pArgs))
        return NULL;

    chanId chid = (chanId) CAPSULE_EXTRACT(pChid, "chid");
    if (chid == NULL)
        return NULL;

    if (!dbr_type_is_valid(dbrtype))
        dbrtype = dbf_type_to_DBR(ca_field_type(chid));

    if (count == 0)
        count = ca_element_count(chid);

    if (PyCallable_Check(pCallback)) {
        ChannelData *pData = new ChannelData(pCallback, pArgs);
        int status = ca_array_get_callback(dbrtype, count, chid, get_callback, pData);
        if (status != ECA_NORMAL) {
            delete pData;
        }
        return Py_BuildValue("(iO)", status, Py_None);
    } else {
        // prepare the storage
        void * pValue = malloc(dbr_size_n(dbrtype, count));
        int status = ca_array_get(dbrtype, count, chid, pValue);
        if (status == ECA_NORMAL) {
            PyObject *dbr = DBRValue_New(dbrtype, count, pValue, false);
            PyObject *pRes = Py_BuildValue("(iO)", status, dbr);
            Py_XDECREF(dbr);
            return pRes;
        } else {
            free(pValue);
            return Py_BuildValue("(iO)", status, Py_None);
        }
    }
}

#define PythonValueToCBuffer(DBRTYPE, FORMAT) \
      pbuf = calloc(count, sizeof(DBRTYPE)); \
      {\
        DBRTYPE *ptr = (DBRTYPE *) pbuf;\
        DBRTYPE value;\
        for(int i=0; i<count; i++){\
          PyObject *item = PySequence_GetItem(pValue, i);\
          PyArg_Parse(item, FORMAT, &value);\
          ptr[i] = (DBRTYPE) value;\
          Py_XDECREF(item);\
        }\
      }

static void put_callback(struct event_handler_args args)
{
    PyGILState_STATE gstate = PyGILState_Ensure();

    ChannelData *pData= (ChannelData *)args.usr;

    if (PyCallable_Check(pData->pCallback)) {
        PyObject *pChid = CAPSULE_BUILD(args.chid, "chid", NULL);
        PyObject *pArgs = Py_BuildValue(
        "({s:O,s:i,s:i,s:i}, O)",
        "chid", pChid,
        "type", args.type,
        "count", args.count,
        "status", args.status,
        pData->pArgs
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


static PyObject *Py_ca_put(PyObject *self, PyObject *args)
{
    PyObject *pChid;
    PyObject *pValue;
    chtype dbrtype;
    unsigned long count = 1;
    PyObject *pCallback = NULL;
    PyObject *pArgs = NULL;
    int status;


    if(!PyArg_ParseTuple(args, "OO|OO", &pChid, &pValue, &pCallback, &pArgs))
        return NULL;

    chanId chid = (chanId) CAPSULE_EXTRACT(pChid, "chid");
    if (chid == NULL)
        return NULL;

    dbrtype = ca_field_type(chid);

    if (PySequence_Check(pValue)) {
        // string input can be written as char array or string
        if (PyBytes_Check(pValue)) {
            count = 1;
            if (dbrtype == DBF_CHAR) {
                count = PySequence_Length(pValue);
            } else if (dbrtype == DBF_ENUM) {
                dbrtype = DBR_STRING;
            }
        } else {
            count = MIN(ca_element_count(chid), PySequence_Length(pValue));
        }
    }

    if (count == 1) {
        db_access_val v;
        switch (dbrtype) {
        case DBR_STRING:
            strncpy(v.strval, PyBytes_AsString(pValue), sizeof(dbr_string_t));
        break;
        case DBR_INT:
            PyArg_Parse(pValue, "h", &v.intval);
        break;
        case DBR_FLOAT:
            PyArg_Parse(pValue, "f", &v.fltval);
        break;
        case DBR_CHAR:
            PyArg_Parse(pValue, "v", &v.charval);
        break;
        case DBR_ENUM:
            PyArg_Parse(pValue, "h", &v.enmval);
        break;
        case DBR_LONG:
            PyArg_Parse(pValue, "i", &v.longval);
        break;
        case DBR_DOUBLE:
            PyArg_Parse(pValue, "d", &v.doubleval);
        break;
        case DBR_PUT_ACKT:
            PyArg_Parse(pValue, "h", &v.putackt);
        break;
        case DBR_PUT_ACKS:
            PyArg_Parse(pValue, "h", &v.putacks);
        break;
        }
        if (PyCallable_Check(pCallback)) {
            ChannelData *pData = new ChannelData(pCallback, pArgs);
            status = ca_put_callback(dbrtype, chid, &v, put_callback, pData);
            if (status != ECA_NORMAL)
                delete pData;
        }
        else
            status = ca_put(dbrtype, chid, &v);

    } else {
        void *pbuf = NULL;

        switch (dbrtype) {
        case DBR_STRING:
        {
            pbuf = calloc(count, sizeof(dbr_string_t));
            dbr_string_t *ptr = (dbr_string_t *) pbuf;
            for(int i=0; i<count; i++) {
                PyObject *item = PySequence_GetItem(pValue, i);
                char *str = NULL;
	            PyArg_Parse(item, "z", &str);
	            strncpy((char *)&(ptr[i]), str, sizeof(dbr_string_t));
                Py_XDECREF(item);
            }
        }
        break;
        case DBR_INT:
            PythonValueToCBuffer(dbr_int_t, "h");
        break;
        case DBR_FLOAT:
            PythonValueToCBuffer(dbr_float_t, "f");
        break;
        case DBR_ENUM:
            PythonValueToCBuffer(dbr_enum_t, "h");
        break;
        case DBR_CHAR:
            PythonValueToCBuffer(dbr_char_t, "b");
        break;
        case DBR_LONG:
            PythonValueToCBuffer(dbr_long_t, "i");
        break;
        case DBR_DOUBLE:
            PythonValueToCBuffer(dbr_double_t, "d");
        break;
        }
        if (PyCallable_Check(pCallback)) {
            ChannelData *pData = new ChannelData(pCallback, pArgs);
            status = ca_array_put_callback(dbrtype, count, chid, pbuf, put_callback, pData);
            if (status != ECA_NORMAL)
                delete pData;
        } else
            status = ca_array_put(dbrtype, count, chid, pbuf);

        if (pbuf)
            free(pbuf);
    }

    return Py_BuildValue("i", status);
}


static PyObject *Py_ca_create_subscription(PyObject *self, PyObject *args)
{
    PyObject *pChid;
    PyObject *pCallback = NULL;
    PyObject *pArgs = NULL;
    chtype dbrtype = -1;
    unsigned long count = 0;
    unsigned long mask = DBE_VALUE | DBE_ALARM;
    if(!PyArg_ParseTuple(args, "OOO|lkk", &pChid,  &pCallback, &pArgs, &dbrtype, &count, &mask))
        return NULL;

    chanId chid = (chanId) CAPSULE_EXTRACT(pChid, "chid");
    if (chid == NULL)
        return NULL;

    if (!dbr_type_is_valid(dbrtype))
        dbrtype = dbf_type_to_DBR(ca_field_type(chid));

    ChannelData *pData = new ChannelData(pCallback, pArgs);

    evid eventID;
    int status;
    Py_BEGIN_ALLOW_THREADS
    status = ca_create_subscription(dbrtype, count, chid, mask, event_callback, pData, &eventID);
    Py_END_ALLOW_THREADS

    if (status == ECA_NORMAL) {
        pData->eventID = eventID;
        PyObject *pEventID = CAPSULE_BUILD(pData, "evid", NULL);
        PyObject *pRes = Py_BuildValue("(iO)", status, pEventID);
        Py_XDECREF(pEventID);
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
        "({s:O,s:i,s:i}, O)",
        "chid", pChid,
        "read_access", args.ar.read_access,
        "write_access", args.ar.write_access,
        pData->pArgs
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
    PyObject *pArgs = NULL;
    if(!PyArg_ParseTuple(args, "OOO", &pChid,  &pCallback, &pArgs))
        return NULL;

    chanId chid = (chanId) CAPSULE_EXTRACT(pChid, "chid");
    if (chid == NULL)
        return NULL;

    /* store the callback and args in channel data */
    ChannelData *pData= (ChannelData *)ca_puser(chid);
    pData->pAccessEventCallback = pCallback;
    pData->pArgs = pArgs;
    Py_XINCREF(pCallback);
    Py_XINCREF(pArgs);

    int status;
    Py_BEGIN_ALLOW_THREADS
    status = ca_replace_access_rights_event(chid, access_rights_handler);
    Py_END_ALLOW_THREADS

    return Py_BuildValue("i", status);
}

static PyObject *pExceptionCallback = NULL;
static PyObject *pExceptionArgs = NULL;

static void exception_handler(struct exception_handler_args args)
{
    PyGILState_STATE gstate = PyGILState_Ensure();

    if (PyCallable_Check(pExceptionCallback)) {
        PyObject *pChid = CAPSULE_BUILD(args.chid, "chid", NULL);
        PyObject *pArgs = Py_BuildValue(
        "({s:O,s:i,s:i,s:i,s:i,s:s,s:s,s:i}, O)",
        "chid", pChid,
        "type", args.type,
        "count", args.count,
        "state", args.stat,
        "op", args.op,
        "ctx", args.ctx,
        "file", args.pFile,
        "lineNo", args.lineNo,
        pExceptionArgs
        );
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
    PyObject *pArgs = NULL;
    if(!PyArg_ParseTuple(args, "OO", &pCallback, &pArgs))
        return NULL;

    /* release previous callback/args */
    Py_XDECREF(pExceptionCallback);
    Py_XDECREF(pExceptionArgs);

    /* store callback/args */
    Py_XINCREF(pCallback);
    Py_XINCREF(pArgs);
    pExceptionCallback = pCallback;
    pExceptionArgs = pArgs;

    int status;
    Py_BEGIN_ALLOW_THREADS
    status = ca_add_exception_event(exception_handler, NULL);
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
    if(!PyArg_ParseTuple(args, "O", &pCallback))
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
    int status = ca_sg_create(&gid);

    return Py_BuildValue("(il)", status, gid);
}

static PyObject *Py_ca_sg_delete(PyObject *self, PyObject *args)
{
    CA_SYNC_GID gid;
    if(!PyArg_ParseTuple(args, "l", &gid))
        return NULL;

    int status = ca_sg_delete(gid);

    return Py_BuildValue("i", status);
}

static PyObject *Py_ca_sg_get(PyObject *self, PyObject *args)
{
    CA_SYNC_GID gid;
    PyObject *pChid;
    chtype dbrtype = -1;
    unsigned long count = 0;
    if(!PyArg_ParseTuple(args, "lO|lk", &gid, &pChid, dbrtype, count))
        return NULL;

    chanId chid = (chanId) CAPSULE_EXTRACT(pChid, "chid");
    if (chid == NULL)
        return NULL;

    if (!dbr_type_is_valid(dbrtype))
        dbrtype = dbf_type_to_DBR(ca_field_type(chid));

    if (count == 0)
        count = ca_element_count(chid);

    // prepare the storage
    void * pValue = malloc(dbr_size_n(dbrtype, (count==0 || count > ca_element_count(chid)) ? ca_element_count(chid) : count));

    int status = ca_sg_array_get(gid, dbrtype, count, chid, pValue);

    if (status == ECA_NORMAL) {
        PyObject *dbr = DBRValue_New(dbrtype, count, pValue, false);
        PyObject *pRes = Py_BuildValue("(iO)", status, dbr);
        Py_XDECREF(dbr);
        return pRes;
    } else {
        free(pValue);
        return Py_BuildValue("(i,O)", status, Py_None);
    }
}

static PyObject *Py_ca_sg_put(PyObject *self, PyObject *args)
{
    CA_SYNC_GID gid;
    PyObject *pChid;
    PyObject *pValue;
    unsigned long count = 1;
    if(!PyArg_ParseTuple(args, "lOO", &gid, &pChid, &pValue))
        return NULL;

    chanId chid = (chanId) CAPSULE_EXTRACT(pChid, "chid");
    if (chid == NULL)
        return NULL;

    chtype dbrtype = ca_field_type(chid);

    if (PySequence_Check(pValue)) {
        // string input can be written as char array or string
        if (PyBytes_Check(pValue)) {
            count = 1;
            if (dbrtype == DBF_CHAR) {
                count = PySequence_Length(pValue);
            } else if (dbrtype == DBF_ENUM) {
                dbrtype = DBR_STRING;
            }
        } else {
            count = MIN(ca_element_count(chid), PySequence_Length(pValue));
        }
    }

    int status;
    if (count == 1) {
        db_access_val v;
        switch (dbrtype) {
        case DBR_STRING:
            strncpy(v.strval, PyBytes_AsString(pValue), sizeof(dbr_string_t));
        break;
        case DBR_INT:
            PyArg_Parse(pValue, "h", &v.intval);
        break;
        case DBR_FLOAT:
            PyArg_Parse(pValue, "f", &v.fltval);
        break;
        case DBR_CHAR:
            PyArg_Parse(pValue, "v", &v.charval);
        break;
        case DBR_ENUM:
            PyArg_Parse(pValue, "h", &v.enmval);
        break;
        case DBR_LONG:
            PyArg_Parse(pValue, "i", &v.longval);
        break;
        case DBR_DOUBLE:
            PyArg_Parse(pValue, "d", &v.doubleval);
        break;
        }
        status = ca_sg_array_put(gid, dbrtype, count, chid, &v);
    } else {
        void *pbuf = NULL;

        switch (dbrtype) {
        case DBR_STRING:
        {
            pbuf = calloc(count, sizeof(dbr_string_t));
            dbr_string_t *ptr = (dbr_string_t *) pbuf;
            for(int i=0; i<=PySequence_Length(pValue); i++) {
                PyObject *item = PySequence_GetItem(pValue, i);
                PyArg_Parse(item, "z", &ptr[i]);
                Py_XDECREF(item);
            }
        }
        break;
        case DBR_INT:
            PythonValueToCBuffer(dbr_int_t, "h");
        break;
        case DBR_FLOAT:
            PythonValueToCBuffer(dbr_float_t, "f");
        break;
        case DBR_ENUM:
            PythonValueToCBuffer(dbr_enum_t, "h");
        break;
        case DBR_CHAR:
            PythonValueToCBuffer(dbr_char_t, "b");
        break;
        case DBR_LONG:
            PythonValueToCBuffer(dbr_long_t, "i");
        break;
        case DBR_DOUBLE:
            PythonValueToCBuffer(dbr_double_t, "d");
        break;
        }
        status = ca_sg_array_put(gid, dbrtype, count, chid, pbuf);
        if (pbuf)
            free(pbuf);
    }

    return Py_BuildValue("i", status);
}

static PyObject *Py_ca_sg_reset(PyObject *self, PyObject *args)
{
    CA_SYNC_GID gid;
    if(!PyArg_ParseTuple(args, "ld", &gid))
        return NULL;

    int status = ca_sg_reset(gid);

    return Py_BuildValue("i", status);
}

static PyObject *Py_ca_sg_block(PyObject *self, PyObject *args)
{
    CA_SYNC_GID gid;
    double timeout;
    if(!PyArg_ParseTuple(args, "ld", &gid, &timeout))
        return NULL;

    int status = ca_sg_block(gid, timeout);

    return Py_BuildValue("i", status);
}

static PyObject *Py_ca_sg_test(PyObject *self, PyObject *args)
{
    CA_SYNC_GID gid;
    if(!PyArg_ParseTuple(args, "l", &gid))
        return NULL;

    int status = ca_sg_test(gid);

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

    chtype field_type = ca_field_type(chid);

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

    unsigned long element_count = ca_element_count(chid);

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

    return PyBytes_FromString(ca_name(chid));
}

static PyObject *Py_ca_state(PyObject *self, PyObject *args)
{
    PyObject *pChid;
    if(!PyArg_ParseTuple(args, "O", &pChid))
        return NULL;

    chanId chid = (chanId) CAPSULE_EXTRACT(pChid, "chid");
    if (chid == NULL)
        return NULL;

    return Py_BuildValue("i", ca_state(chid));
}
static PyObject *Py_ca_host_name(PyObject *self, PyObject *args)
{
    PyObject *pChid;
    if(!PyArg_ParseTuple(args, "O", &pChid))
        return NULL;

    chanId chid = (chanId) CAPSULE_EXTRACT(pChid, "chid");
    if (chid == NULL)
        return NULL;

    return PyBytes_FromString(ca_host_name(chid));
}

static PyObject *Py_ca_read_access(PyObject *self, PyObject *args)
{
    PyObject *pChid;
    if(!PyArg_ParseTuple(args, "O", &pChid))
        return NULL;

    chanId chid = (chanId) CAPSULE_EXTRACT(pChid, "chid");
    if (chid == NULL)
        return NULL;

    return Py_BuildValue("i", ca_read_access(chid));
}

static PyObject *Py_ca_write_access(PyObject *self, PyObject *args)
{
    PyObject *pChid;
    if(!PyArg_ParseTuple(args, "O", &pChid))
        return NULL;

    chanId chid = (chanId) CAPSULE_EXTRACT(pChid, "chid");
    if (chid == NULL)
        return NULL;

    return Py_BuildValue("i", ca_write_access(chid));
}


/*******************************************************
 *                    Utility                          *
 *******************************************************/

static PyObject *Py_dbf_text(PyObject *self, PyObject *args)
{
    chtype field_type;

    if(!PyArg_ParseTuple(args, "l", &field_type))
        return NULL;

    return PyBytes_FromString(dbf_type_to_text(field_type));
}

static PyObject *Py_dbr_text(PyObject *self, PyObject *args)
{
    chtype req_type;

    if(!PyArg_ParseTuple(args, "l", &req_type))
         return NULL;

    return PyBytes_FromString(dbr_type_to_text(req_type));
}

static PyObject *Py_ca_message(PyObject *self, PyObject *args)
{
    int status;

    if(!PyArg_ParseTuple(args, "i", &status))
         return NULL;

    return PyBytes_FromString(ca_message(status));
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

#define TS2secs(ts) ((double)ts.secPastEpoch + (double) ts.nsec*1.0e-9)

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
        if(use_numpy == 0 || with_numpy == 0)
            FormatPlaintoValue(count, val, dbr_short_t,  PyInt_FromLong)
        #ifdef WITH_NUMPY
        else
            FormatPlaintoValueArray(count, val, dbr_short_t,  PyInt_FromLong,  NPY_SHORT)
        #endif
        arglist = Py_BuildValue("O", value);
        break;
    case DBR_FLOAT:
        if(use_numpy == 0 || with_numpy == 0)
            FormatPlaintoValue(count, val, dbr_float_t,  PyFloat_FromDouble)
        #ifdef WITH_NUMPY
        else
            FormatPlaintoValueArray(count, val, dbr_float_t,  PyFloat_FromDouble,  NPY_FLOAT)
        #endif
        arglist = Py_BuildValue("O", value);
        break;
    case DBR_ENUM:
        if(use_numpy == 0 || with_numpy == 0)
            FormatPlaintoValue(count, val, dbr_enum_t,   PyInt_FromLong)
        #ifdef WITH_NUMPY
        else
            FormatPlaintoValueArray(count, val, dbr_enum_t,  PyInt_FromLong,  NPY_USHORT)
        #endif
        arglist = Py_BuildValue("O", value);
        break;
    case DBR_CHAR:
        if (use_numpy == 0 || with_numpy == 0)
            FormatPlaintoValue(count, val, dbr_char_t,   PyInt_FromLong)
        #ifdef WITH_NUMPY
        else
            FormatPlaintoValueArray(count, val, dbr_char_t,   PyInt_FromLong,  NPY_BYTE)
        #endif
        arglist = Py_BuildValue("O", value);
        break;
    case DBR_LONG:
        if (use_numpy == 0 || with_numpy == 0)
            FormatPlaintoValue(count, val, dbr_long_t,   PyInt_FromLong)
        #ifdef WITH_NUMPY
        else
            FormatPlaintoValueArray(count, val, dbr_long_t,   PyInt_FromLong,  NPY_INT)
        #endif
        arglist = Py_BuildValue("O", value);
        break;
    case DBR_DOUBLE:
        if (use_numpy == 0 || with_numpy == 0)
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
        if (use_numpy == 0 || with_numpy == 0)
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
        if (use_numpy == 0 || with_numpy == 0)
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
        if (use_numpy == 0 || with_numpy == 0)
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
        if (use_numpy == 0 || with_numpy == 0)
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
        if (use_numpy == 0 || with_numpy == 0)
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
        if (use_numpy == 0 || with_numpy == 0)
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
        if (use_numpy == 0 || with_numpy == 0)
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
        if (use_numpy == 0 || with_numpy == 0)
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
        if (use_numpy == 0 || with_numpy == 0)
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
        if (use_numpy == 0 || with_numpy == 0)
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
        if (use_numpy == 0  || with_numpy == 0)
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
        if (use_numpy == 0  || with_numpy == 0)
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
        if (use_numpy == 0  || with_numpy == 0)
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
        if (use_numpy == 0  || with_numpy == 0)
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
        if (use_numpy == 0 || with_numpy == 0)
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
        if (use_numpy == 0 || with_numpy == 0)
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
        if (use_numpy == 0 || with_numpy == 0)
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
        if (use_numpy == 0 || with_numpy == 0)
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
        if (use_numpy == 0 || with_numpy == 0)
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
        if (use_numpy == 0 || with_numpy == 0)
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
        if (use_numpy == 0 || with_numpy == 0)
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
        arglist=Py_BuildValue("{s:O,s:i,s:i,s:i,s:O)",
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
        if (use_numpy == 0 || with_numpy == 0)
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
        if (use_numpy == 0 || with_numpy == 0)
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
        if (use_numpy == 0 || with_numpy == 0)
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
        arglist = Py_BuildValue("s:i,s:i,s:i,s:i,s:s",
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
