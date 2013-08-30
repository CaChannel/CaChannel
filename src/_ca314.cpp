/* Python-EPICS channel access interface module  */
/* #ver1.1-1.5 written by Noboru Yamamoto, KEK, JAPAN */
/* modified to adopt EPICS.3.14.x , including threading */
/*for installation
1) check setup.py for environment. 
2) run 
  python setup.py build
  python setup.py install
*/

const char * _Py_ca_version="$Revision: 1.16 $";

#if (defined( __GNUC__ ) && (__GNUC__ >= 3)) || defined(Darwin) || defined(HP_UX) || defined(WIN32)
#  include <stdarg.h>
#else
#  include <varargs.h>
#endif

#if defined( __GNUC__ )
# if !defined(INLINE)
#  define INLINE inline
# endif
#else
# if !defined(INLINE)
#  define INLINE
# endif
#endif

#if defined(WITH_TK) 
#include <tk.h>
/* borrowed from Python/Modules/_tkinter.c */
#  if !(defined(MS_WINDOWS) || defined(__CYGWIN__) || defined(WITH_THREAD))
#    define HAVE_CREATEFILEHANDLER
#  endif
#endif 

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>

#include <epicsThread.h>
#include <epicsMutex.h>
#include <epicsEvent.h>

#include "Python.h"
#include "pythread.h"

#if PY_VERSION_HEX < 0x02050000
#define PyErr_WarnEx(category, msg, level) PyErr_Warn(category, msg)
#endif
 
#if PY_MAJOR_VERSION >= 3
#define PyString_FromString PyBytes_FromString
#endif

//#define DEBUG 1
#define  DEBUG 0
// make sure asser() to work.
#ifdef NDEBUG
#undef NDEBUG
#endif

#include <epicsVersion.h>
#include <errlog.h>
#include <caerr.h>
#include <cadef.h>
/* ca_detach_contex is in library but not in header files. */
/* ca_detach_contex requires EPICS R3.14.8 or later **/
epicsShareFunc void epicsShareAPI ca_detach_context () ;

#define CA_PEND_EVENT_TIME	1e-3		/* formerly 0.0001    */
#define CA_MAX_RETRY 300

#define TS2secs(ts) ((double)ts.secPastEpoch + (double) ts.nsec*1.0e-9)

/* Python/Tk/EPICS-CA needs thread lock to be thread ready */
/* routines in _ca.c may be called from ether Python interpreter or
   Tkinter context as a result of callback. */
static struct ca_client_context *my_cac=NULL;

#if !defined(WITH_THREAD) /* you HAVE to use thread in EPICS 3.14 or later , 2005, Jan. 12 */
#  define WITH_THREAD
#endif

#define ENTER_CA \
  if(my_cac){ca_attach_context(my_cac);{
 
#define LEAVE_CA \
  } ca_detach_context();}

/* function prototypes */
static void exceptionCallback(struct exception_handler_args args);
static int exceptionCallbackFormated(long, const char *, int, const char *, ...);

/* entry point for Python module initializer */
extern "C"{ void init_ca(void); } /* otherwise python cannot find the entry point to this module. */

static void ca_context_destroy_wrapper(void);
static PyObject *GetValfromArgs(struct event_handler_args args);
static PyObject *_convert_ca_to_Python(chtype type, unsigned long count, void *pvalue, int satus);
static int       _convert_Py_to_ca(chtype type, unsigned long count, PyObject *val, void *pbuf);

static PyObject *Py_ca_search(PyObject *self, PyObject *args);
static PyObject *Py_ca_clear(PyObject *self, PyObject *args);
static PyObject *Py_ca_get(PyObject *self, PyObject *args);
static PyObject *Py_ca_put(PyObject *self, PyObject *args);
static PyObject *Py_ca_monitor(PyObject *self, PyObject *args);
static PyObject *Py_ca_clear_monitor(PyObject *self, PyObject *args);
static PyObject *Py_ca_pend_io(PyObject *self, PyObject *args);
static PyObject *Py_ca_test_io(PyObject *self, PyObject *args);
static PyObject *Py_ca_pend_event(PyObject *self, PyObject *args);
static PyObject *Py_ca_poll(PyObject *self, PyObject *args);
static PyObject *Py_ca_flush(PyObject *self, PyObject *args);
static PyObject *Py_ca_status(PyObject *self, PyObject *args);
static PyObject *Py_add_fd_registration(PyObject *self, PyObject *args);
static PyObject *Py_ca_channelInfo(PyObject *self, PyObject *args);
static PyObject *Py_ca_fileno(PyObject *self, PyObject *args);

static PyObject *Py_sg_create(PyObject *self, PyObject *args);
static PyObject *Py_sg_delete(PyObject *self, PyObject *args);
static PyObject *Py_sg_block(PyObject *self, PyObject *args);
static PyObject *Py_sg_test(PyObject *self, PyObject *args);
static PyObject *Py_sg_reset(PyObject *self, PyObject *args);
static PyObject *Py_sg_put(PyObject *self, PyObject *args);
static PyObject *Py_sg_get(PyObject *self, PyObject *args);
static PyObject *Py_ca_convert(PyObject *self, PyObject *args);
static PyObject *Py_ca_task_exit(PyObject *self, PyObject *args);

static PyObject *Py_ca_v42_ok(PyObject *self, PyObject *args);
static PyObject *Py_ca_version(PyObject *self, PyObject *args);

static PyObject *Py_ca_name(PyObject *self, PyObject *args);
static PyObject *Py_dbf_text(PyObject *self, PyObject *args);
static PyObject *Py_dbr_text(PyObject *self, PyObject *args);
 
/* for Tk-registration */
#if defined(HAVE_CREATEFILEHANDLER)
static PyObject *Py_ca_register_to_Tk(PyObject *self, PyObject *args);
static void Tk_ca_fd_register(void *pfdctx,int	fd,int condition);
static void Tk_ca_service(void *, int);
#endif 

/*  Method Table  */
static PyMethodDef CA_Methods[]={
  /* name in Python, function, flag(1 always), doc(str*) */
  {"search", Py_ca_search, METH_VARARGS, "EPICS CA search_and_connect"}, 
  {"clear", Py_ca_clear, METH_VARARGS, "EPICS CA channel close"}, 
  {"get", Py_ca_get, METH_VARARGS, "EPICS CA: get value in native form"}, 
  {"put", Py_ca_put, METH_VARARGS, "EPICS CA:put value"}, 
  {"monitor", Py_ca_monitor, METH_VARARGS, "EPICS CA:set up monitor callback"}, 
  {"clear_monitor", Py_ca_clear_monitor, METH_VARARGS, "EPICS CA:delete monitor callback"}, 
  {"pendio", Py_ca_pend_io, METH_VARARGS, "EPICS CA: wait for io completion"}, 
  {"pend_io", Py_ca_pend_io, METH_VARARGS, "EPICS CA: wait for io completion"}, 
  {"test_io", Py_ca_test_io, METH_VARARGS, "EPICS CA: test outstandig IO queries."}, 
  {"pend_event", Py_ca_pend_event, METH_VARARGS, 
   "EPICS CA: wait for next event(monitor, Archive, Alarm)"}, 
  {"poll", Py_ca_poll, METH_NOARGS, "EPICS CA: poll CA event(monitor, Archive, Alarm)"}, 
  {"flush", Py_ca_flush, METH_VARARGS, "EPICS CA: flush CA buffer"}, 
  {"status", Py_ca_status, METH_VARARGS, "EPICA CA:connection status(0:connected)"}, 
  {"ch_info", Py_ca_channelInfo, METH_VARARGS, "EPICS CA: channel info"}, 
  {"fileno", Py_ca_fileno, METH_VARARGS, "EPICS CA: socket id, obsolete "}, 
  {"add_fd_registration", Py_add_fd_registration, METH_VARARGS, 
   "Add file descriptor registration routine. args:handler, handler_args"}, 
  {"sg_create", Py_sg_create, METH_VARARGS, "create sync. group id"}, 
  {"sg_delete", Py_sg_delete, METH_VARARGS, "delete sync. group id"}, 
  {"sg_block", Py_sg_block, METH_VARARGS, "wait for sync group operation"}, 
  {"sg_test", Py_sg_test, METH_VARARGS, "test completion of sync group operation."}, 
  {"sg_reset", Py_sg_reset, METH_VARARGS, "reset a sync group status"}, 
  {"sg_put", Py_sg_put, METH_VARARGS, "issue a put request in a sync group"}, 
  {"sg_get", Py_sg_get, METH_VARARGS, "issue a get request in a sync group"}, 
  {"ca_convert", Py_ca_convert, METH_VARARGS, "Convert value returned by CA to PyObject"}, 
  {"ca_version", Py_ca_version, METH_NOARGS, "retrun the CA version string"}, 
  {"ca_v42_ok", Py_ca_v42_ok, METH_VARARGS, "channel is server by a v42 server"}, 
  {"__ca_task_exit", Py_ca_task_exit, METH_VARARGS, "Terminate CA libary"}, 
  /* the following 3 lines by Wang */
  {"name", Py_ca_name, METH_VARARGS, "EPICS CA: channel name"}, 
  {"dbf_text", Py_dbf_text, METH_VARARGS, "field name from index" },  
  {"dbr_text", Py_dbr_text, METH_VARARGS, "request type name from index"}, 
#if defined(HAVE_CREATEFILEHANDLER)
  {"ca_register_to_Tk", Py_ca_register_to_Tk, METH_VARARGS, "register Channel to Tk_file_handler"}, 
#endif
  {NULL, NULL, 0, NULL}
};

/* error objects */
static PyObject *CaError=NULL;

/* glue routines to call Python code as callback */
static void  get_callback(struct event_handler_args );

static void  exec_callback(struct connection_handler_args);
static void  con_change_callback(struct connection_handler_args);

enum purgebleT{DontPurge=0, Purgeble=1};
 
class  _ca_frame{
private:
  PyThread_type_lock mutex;

public:
  evid EVID;
  PyObject *pfunc; 
  PyObject *args;
  PyThreadState *tstate;
  purgebleT purgeble; /* this callback is used only once */
  
  _ca_frame(evid EVID, PyObject *pfunc, PyObject *args, PyThreadState *tstate, purgebleT purgeble=DontPurge);
  ~_ca_frame();
  int lock(int wait=WAIT_LOCK);
  int unlock();
};


_ca_frame::_ca_frame(evid EVID, PyObject *pfunc, PyObject *args, 
 		     PyThreadState *tstate, purgebleT purgeble)
{
  int status=0;

  if(DEBUG)
    PyErr_WarnEx(NULL, "_ca_frame:crete frame", 2);

  this->mutex = PyThread_allocate_lock();
  Py_BEGIN_ALLOW_THREADS{
    ENTER_CA{
      // we wants a locked mutex here.
      status=PyThread_acquire_lock(this->mutex, NOWAIT_LOCK);
    }LEAVE_CA;
  }Py_END_ALLOW_THREADS;

  if (status == 0 ){
    PyErr_WarnEx(NULL, 
                 "ca_freame_lock:failed to lock\n", 1);
  }
  this->pfunc=pfunc;  Py_XINCREF(pfunc);
  this->args=args;    Py_XINCREF(args);
  this->EVID=EVID;
  this->tstate=tstate;
  this->purgeble=purgeble;
}

int _ca_frame::lock(int wait ){ /* wait=WAIT_LOCK is the default */
  int status=0;

  Py_BEGIN_ALLOW_THREADS{
    ENTER_CA{
      status=PyThread_acquire_lock(this->mutex, wait);
    }LEAVE_CA;
  }Py_END_ALLOW_THREADS;

  if (status == 0){
    PyErr_WarnEx(NULL, "ca_frame_lock already locked\n", 2);
    return -1;
  }
  else {
    if (DEBUG) PyErr_WarnEx(NULL, "ca_frame_lock locked\n", 2);
    return 0;
  }
}

int _ca_frame::unlock(){
  if (DEBUG) PyErr_WarnEx(NULL, "unlock ca_frame \n", 2);
  PyThread_release_lock(this->mutex);
  return 0;
}

_ca_frame::~_ca_frame(){
  int status;

  if (DEBUG)   PyErr_WarnEx(NULL, "release ca_frame\n", 1);
  Py_XDECREF(this->pfunc);
  Py_XDECREF(this->args);
  this->pfunc=NULL;
  this->args =NULL;
  this->tstate=NULL;

  assert(this->mutex);
  /* Unlock the lock so it's safe to free it */
  if (DEBUG)  PyErr_WarnEx(NULL, "ca_frame_lock try lock it before freeing\n", 2);
  Py_BEGIN_ALLOW_THREADS{
    status=PyThread_acquire_lock(this->mutex, NOWAIT_LOCK);
  }Py_END_ALLOW_THREADS;
  if (status == 0 && DEBUG){
    PyErr_WarnEx(NULL, "ca_frame_lock was already locked\n", 2);
  }
  PyThread_release_lock(this->mutex);
  PyThread_free_lock(this->mutex);
  this->mutex=NULL;
}

/* initialization routinoe */
/* module name and a initialization routine are registered in the inittab record in PyConfig.c */

/* ca_state becomes a function not a macro. We must enclose it  */

INLINE static int pyCA_Ensure_Connection( chid _ch_id, const char *_fname) {
  /* this function has to be called in EPICS-CA context */
  int status=ECA_NORMAL;

  if(!(_ch_id)){
    status = ECA_BADCHID;
    return status;
  }
  else if( ca_state((_ch_id)) == cs_closed ) {
    status = ECA_DISCONN;
    return status;
  }
  if(ca_state((_ch_id)) != cs_conn ) {
    try{
      errlogPrintf("wait for connection.\n");
      status=ca_pend_event(0.03); /* wait for ch_id to connect */ 
    }
    catch(...){
      errlogPrintf("exception while pend_event \n");
      status=ECA_TIMEOUT;
    }
    if (status != ECA_NORMAL){
      errlogPrintf("connection failed\n");
      return status;
    }
  }

  if( ca_state(_ch_id) != cs_conn ) { /* still not connected */
    errlogPrintf("shoud have connected.\n");
    status=ECA_TIMEOUT;
  }
  return status;
}

static PyObject *saved_exitfunc=NULL;

static void ca_context_destroy_wrapper(void)
{
  Py_BEGIN_ALLOW_THREADS{
    ENTER_CA{
      if(ca_current_context() ){
	ca_context_destroy();
      }
    }LEAVE_CA;
  }Py_END_ALLOW_THREADS;
}

static PyObject *Py_ca_task_exit(PyObject *self, PyObject *args)
{
  static int task_closed=0;
  PyObject *res=NULL;

  // You should not leave Python_Thread at this point.
  if (saved_exitfunc && PyCallable_Check(saved_exitfunc)){
    Py_XINCREF(saved_exitfunc);
    try{
      res=PyObject_CallObject(saved_exitfunc, (PyObject *) NULL);
    }
    catch(...){
      PyErr_WarnEx(NULL, "exception in callback\n", 1);
    }
    Py_XDECREF(saved_exitfunc);
    if (res == NULL && (PyErr_Occurred() != NULL)){
      PyErr_Print();
    }
  }
  if (!task_closed){
    Py_BEGIN_ALLOW_THREADS{
      if(my_cac){
	  task_closed=1;
	  ca_attach_context(my_cac);
	  ca_task_exit(); // it calles callback function for each channel. and try to lock 
	  ca_context_destroy(); 
	  my_cac=NULL;
      }
    }Py_END_ALLOW_THREADS;
  }
  else{
    PyErr_WarnEx(NULL, "CA taks is already closed.", 2);
  }
  Py_XINCREF(Py_None);
  return Py_None;
}

#define EPICS_CA_ENABLE_PREEMPTIVE "EPICS_CA_ENABLE_PREEMPTIVE"

extern "C"{
  void init_ca(void){
    int status;
    PyObject *m, *d, *obj;
    PyObject *caexit, *exitfunc=PySys_GetObject((char *) "exitfunc");
    enum ca_preemptive_callback_select ca_thread_preemptive_option = ca_enable_preemptive_callback; 

//     if(DEBUG)
//       PyErr_WarnEx(NULL, "Initialize _ca Module", 2);

    if( ! PyEval_ThreadsInitialized()){
      PyErr_WarnEx(NULL, "PyEval_InitThreads should be called in threadmodule.", 1);
      PyEval_InitThreads();
    }
    
    m=Py_InitModule("_ca", CA_Methods);
    d=PyModule_GetDict(m);
    CaError = PyErr_NewException((char *) "_ca.error", NULL, NULL);
    PyDict_SetItemString(d, "calibError", CaError);
    PyDict_SetItemString(d, "CAlibError", CaError);
    PyDict_SetItemString(d, "CaError", CaError);
    PyDict_SetItemString(d, "error", CaError);
    
    obj=PyString_FromString(_Py_ca_version);
    PyDict_SetItemString(d, "version", obj);
    Py_DECREF(obj);                            
#if defined(PYCA_VERSION)
    obj=PyString_FromString(PYCA_VERSION);
#else
    obj=PyString_FromString("unknown");
#endif
    PyDict_SetItemString(d, "release", obj);
    PyDict_SetItemString(d, "revision", obj);
    Py_DECREF(obj);                            

    /* CA initialization */
    /* set up file descriptor mgr and initialize channel access */
    status=-1;
    Py_BEGIN_ALLOW_THREADS{
      status=ca_context_create(ca_thread_preemptive_option);
      if (status == ECA_NORMAL){
	my_cac = ca_current_context();
      }
      else{
	if (DEBUG)       
	  errlogPrintf("failed create ca_context\n");
      }
      SEVCHK(status, "init_ca:ca_context_create");
      ca_detach_context () ;
    }Py_END_ALLOW_THREADS;

    if (status != ECA_NORMAL){
      PyErr_SetString(CaError, 
		      "init_ca: C.A. initialize failure.\n");
    }

    Py_XINCREF(exitfunc);
    saved_exitfunc=exitfunc;
    caexit=PyDict_GetItemString(d, "__ca_task_exit");
    
    Py_XINCREF(caexit);
    PySys_SetObject((char *) "exitfunc", caexit);
    
    Py_BEGIN_ALLOW_THREADS{
      ENTER_CA{
	status=ca_add_exception_event(exceptionCallback, NULL);
      }LEAVE_CA;
    }Py_END_ALLOW_THREADS;
    if(status!=ECA_NORMAL){
      PyErr_SetString(CaError, 
		      "init_ca: failed to register exception handler\n");
    }
  }
} //extern 

static PyObject *
Py_ca_search(PyObject *self, PyObject *args)
{
  static char *name;
  chid ch_id;
  int status=-1;
  _ca_frame *pframe=NULL;
  
  unsigned long nc;
  PyObject *pcallback;
  
  if(!PyArg_ParseTuple(args, "z#O", &name, &nc, &pcallback))
    return NULL;
  
  if (!name || !nc ) {
    PyErr_SetString(CaError, "Empty channel Name");
    return NULL; /* NULL triggers Exception */
  }

  Py_XINCREF(pcallback);
  {
    if (! PyCallable_Check(pcallback) ){
      Py_BEGIN_ALLOW_THREADS{
	ENTER_CA {
	  status = ca_create_channel(name, NULL, NULL, 0 , &ch_id);
	} LEAVE_CA;
      }Py_END_ALLOW_THREADS;
    }
    else{ 
      pframe=new _ca_frame(NULL, pcallback, Py_None, PyThreadState_Get(), DontPurge);
      /* this cb is called when connection status changes.  
	 so, do not delete this callback 
      */
      Py_XINCREF(pcallback);
      Py_BEGIN_ALLOW_THREADS{
	ENTER_CA {
	  const int priority=0;
	  status = ca_create_channel(name, exec_callback, pframe, priority , &ch_id);
	} LEAVE_CA;
      }Py_END_ALLOW_THREADS;
      Py_XDECREF(pcallback);
      pframe->unlock();
    }
  }
  Py_XDECREF(pcallback);
  
  if (status != ECA_NORMAL){
    PyErr_SetString(CaError, ca_message(status));
    return NULL;
  }
  else {
    if (PyCallable_Check(pcallback)) {
      Py_BEGIN_ALLOW_THREADS{
	ENTER_CA{
	  status=ca_change_connection_event(ch_id, con_change_callback);
	}LEAVE_CA;
      }Py_END_ALLOW_THREADS;
      if (status != ECA_NORMAL){
	PyErr_SetString(CaError, ca_message(status));
	return NULL;
      }
    }
    return Py_BuildValue("l", ch_id);
  }
}

static PyObject *
Py_ca_clear(PyObject *self, PyObject *args){

  chid ch_id;
  int status=-1;
  int pyca_state=0;
  const char * pyca_errmsg=NULL;

  if(!PyArg_ParseTuple(args, "l", &ch_id))
    return NULL;

  if (!ch_id ) {
    PyErr_SetString(CaError, "Null channel ID as an argument");
    return NULL; /* NULL trigers Exception */ 
  }

  Py_BEGIN_ALLOW_THREADS{
    ENTER_CA{
	pyca_state=ca_state(ch_id);
    }LEAVE_CA;
  }Py_END_ALLOW_THREADS;

  if (pyca_state == cs_never_conn ||
      pyca_state == cs_closed ) {
    PyErr_SetString(CaError, "Channel not connected or already connected");
    return NULL; /* NULL trigers Exception */ 
  }

  Py_BEGIN_ALLOW_THREADS{
    ENTER_CA{
      status=ca_clear_channel(ch_id);
      //SEVCHK(status, "Py_ca_clear");
      pyca_errmsg=ca_message(status);
    }LEAVE_CA;
  }Py_END_ALLOW_THREADS;
 
  if(status != ECA_NORMAL){
    PyErr_SetString(CaError, pyca_errmsg);
    return NULL;
  }
  return Py_BuildValue("i", status);
} 

static PyObject *
Py_ca_get(PyObject *self, PyObject *args){
  chid ch_id;
  int status=ECA_NORMAL;
  unsigned long ecount;
  chtype type;
  const char *ca_errmsg=NULL;
  _ca_frame *pframe=NULL;

  PyObject *pcallback, *rval;
  PyThreadState *my_thread_state;

  if(!PyArg_ParseTuple(args, "lOll", &ch_id, &pcallback, &type, &ecount))
    return NULL;
  
  Py_BEGIN_ALLOW_THREADS{
    ENTER_CA{
      if (DEBUG)
	errlogPrintf("set up callback for get. requested data type is %d\n", 
		     ca_field_type(ch_id));
      status=pyCA_Ensure_Connection(ch_id, "Py_ca_get"); /* MACRO for channel connection checking */
      ca_errmsg=ca_message(status);
    }LEAVE_CA;
  }Py_END_ALLOW_THREADS;
  if (status == ECA_NORMAL){
    my_thread_state=PyThreadState_Get();
    pframe=new _ca_frame(NULL, pcallback, Py_None, my_thread_state, Purgeble);
    Py_BEGIN_ALLOW_THREADS{
      ENTER_CA{
	if (type == -1){
	  type = dbf_type_to_DBR_TIME(ca_field_type(ch_id));
	}
	if (ecount==0){
	  ecount=ca_element_count(ch_id);
	}
	{
	  status=ca_array_get_callback(type, ecount, 
				       ch_id, get_callback, pframe);
	}
	ca_errmsg=ca_message(status);
      }LEAVE_CA;
    }Py_END_ALLOW_THREADS;
  }
  else{
    pframe=NULL;
  }
  if(status != ECA_NORMAL){
    PyErr_SetString(CaError, ca_errmsg);
    rval=NULL;
  }
  else{
    rval=Py_BuildValue("i", status);
  }
  if (pframe != NULL) pframe->unlock();
  return rval;
}

typedef dbr_string_t *dbr_string_t_ptr;
typedef dbr_char_t *dbr_char_t_ptr ;

#define MoveDBRtoValue(DBRTYPE, VP, PYTYPE, FORMAT) \
     {\
       struct dbr_sts_##DBRTYPE  *cval=(struct dbr_sts_##DBRTYPE  *)val;\
       register dbr_##DBRTYPE##_t *vp=(dbr_##DBRTYPE##_t *)(VP);\
       register unsigned long cur;\
       arglist=Py_BuildValue("((Oii))", array, \
			     (int) cval->severity, (int) status);\
       for (cur=0;cur < count; cur++){\
	 PyObject *ent=Py_BuildValue(FORMAT, (PYTYPE) *vp++);\
	 PySequence_SetSlice(array, cur, cur, ent);\
	 Py_XDECREF(ent);\
       }\
       Py_XDECREF(array);\
     } // we need these DECREFs. Otherwise memory leak occures for array put

#define MoveTimeDBRtoValue(DBRTYPE, VP, PYTYPE, FORMAT) \
     {\
       struct dbr_time_##DBRTYPE  *cval=(struct dbr_time_##DBRTYPE  *)val;\
       register dbr_##DBRTYPE##_t *vp=(dbr_##DBRTYPE##_t *)(VP);\
       register unsigned long cur;\
       arglist=Py_BuildValue("((Oiid))", array, \
			     (int) cval->severity, (int) status, \
			     TS2secs(cval->stamp));\
       for (cur=0;cur < count; cur++){\
	 PyObject *ent=Py_BuildValue((FORMAT), (PYTYPE) *vp++);\
	 PySequence_SetSlice(array, cur, cur, ent);\
	 Py_XDECREF(ent);\
       }\
       Py_XDECREF(array);\
     }

/* Dbx_xxx_yyy structures are defined in db_access.h in EPICS/base/include */

static void get_callback(struct event_handler_args args)
{
  int status;
  PyObject *pcallback=NULL, *arglist=NULL, *result=NULL;
  _ca_frame *pframe=NULL;
  PyThreadState *cbtstate=NULL, *oldtstate=NULL;
  PyGILState_STATE gstate;

  /* check ca_contex */
  if ( ca_current_context() == NULL){
    if (my_cac) {
      if (DEBUG){
	errlogPrintf("ca_context is NULL -> switched to my ca-context.\n");
      }
      ca_attach_context(my_cac);
    }
    else{
      errlogPrintf("Null ca_context in get_callback.\n");
    }
  }
  else if(ca_current_context() !=  my_cac){
    errlogPrintf("ca_context:different from my ca-context.\n");    
  }
  
  /*restore thread state to that of callback is defined */
  status=args.status;
  pframe=(_ca_frame *) args.usr;

  if(my_cac) ca_detach_context(); /*Leave EPICS World */
  {
    gstate = PyGILState_Ensure(); /* Go to Python World */
    
    arglist=result=NULL;
    try{
      if (pframe == NULL){
	PyErr_WarnEx(NULL, "missing execution environment(pframe)\n", 1);
	return;
      }
      pframe->lock();
      {
	cbtstate=pframe->tstate;
	try{
	  if(cbtstate != NULL) {
#if defined(DEBUG) && (DEBUG >1)
	    PyErr_WarnEx(NULL, "restore thread state in get callback\n", 1);
#endif 
	    //oldtstate=PyThreadState_Swap(cbtstate);
	    //oldtstate=NULL;
	  }
	  else{
	    oldtstate=NULL;
	    //PyErr_WarnEx(NULL, " Empty thread state in callback\n", 1);
	  } 
	  pcallback=pframe->pfunc;
	  if (args.dbr == NULL || status != ECA_NORMAL){
	    arglist=Py_BuildValue("((Oi))", Py_None, status);
	  }
	  else{
	    arglist=GetValfromArgs(args);
	    if (arglist == (PyObject *) NULL){
	      arglist=PyTuple_New(0);
	      Py_XINCREF(arglist);
	    }
	  }
	  /* call callback routine with arglist -> do whatever it likes */
	  
	  if (PyCallable_Check(pcallback)){
#if defined( DEBUG ) && (DEBUG > 1)
	    PyErr_WarnEx(NULL, "]] calling Python callback\n", 1);fflush(stderr);
#endif
	    Py_XINCREF(pcallback); 
	    try{
	      result=PyObject_CallObject(pcallback, arglist);
	    }
	    catch(...){
#if defined( DEBUG ) && (DEBUG > 1)
	      PyErr_WarnEx(NULL, "[[Python callback failed\n", 1);fflush(stderr);
#endif
	    }
#if defined( DEBUG ) && (DEBUG > 1)
	    PyErr_WarnEx(NULL, "[[ done Python callback\n", 1);fflush(stderr);
#endif
	    Py_XDECREF(pcallback); 
	  }
#if defined( DEBUG ) && (DEBUG > 1)
	  else{
	    PyErr_WarnEx(NULL, "CallBack is not callable\n", 1);fflush(stderr);
	  }
#endif
	  if (oldtstate) {
	    //cbtstate=PyThreadState_Swap(oldtstate);
	  }
	}
	catch(...){
	  PyErr_WarnEx(NULL, "exception in callback\n", 1);fflush(stderr);
	}
      }
      
      Py_XDECREF(result); 
      Py_XDECREF(arglist); 
      
      if (pframe->purgeble == 1) {
	delete pframe; 
	pframe=NULL;
      }
      else{
	pframe->unlock();
      }
    }
    catch(...){
      PyErr_WarnEx(NULL, "exception in callback\n", 1);fflush(stderr);
    }
    
    PyGILState_Release(gstate); /* Leaving Python world */
    if (my_cac) {
      ca_attach_context(my_cac); /* return to EPICS world */
    }
  }
  return;
}

static PyObject *GetValfromArgs(struct event_handler_args args){
  PyObject *retObj; 
  /*errlogPrintf("call back get %ld number of elements\n", args.count);*/
  retObj=_convert_ca_to_Python(args.type, args.count, 
			       (void *) args.dbr, args.status);
  return retObj;
}
/* helper function by Wang-san */
#if 1
#  include "_conv_helper.cpp"
#else /* older/original version. It should go away soon. */
static PyObject *
_convert_ca_to_Python(chtype type, unsigned long count, void *val, int status){
  PyObject *arglist = NULL;
  PyObject *array   = NULL;

  if(!val){
    if(CaError){
      PyErr_SetString(CaError, "Null pointer as return value.");  
    }
    return (PyObject *) NULL;
  }
  else{
    if(count==1){
      switch(type){
      case DBR_STRING:
	arglist=Py_BuildValue("((si))", ((char *) val), status);
	break;
      case DBR_INT:
      case DBR_ENUM:
	arglist=Py_BuildValue("((ii))", *((short *) val), status);
	break;
      case DBR_FLOAT:
	arglist=Py_BuildValue("((fii))", *((float *) val), -1, status);
	break;
      case DBR_CHAR:
	arglist=Py_BuildValue("((bii))", ((char *) val), -1, status);
	break;
      case DBR_LONG:
	arglist=Py_BuildValue("((lii))", *((long *) val), -1, status);
	break;
      case DBR_DOUBLE:
	arglist=Py_BuildValue("((dii))", *((double *) val), -1, status);
	break;
      case DBR_TIME_STRING:
	{
	  struct dbr_time_string *cval=(struct dbr_time_string *) val;
	  arglist=Py_BuildValue("((siid))", 
				cval->value, cval->severity, cval->status, 
				TS2secs(cval->stamp));
	}
	break;
	/*   case DBR_TIME_INT: */
       case DBR_TIME_SHORT:	
	{
	  struct dbr_time_short  *cval=(struct dbr_time_short  *) val;
	  arglist=Py_BuildValue("((iiid))", 
				cval->value, cval->severity, cval->status, 
				TS2secs(cval->stamp));
	}
	break;
      case DBR_TIME_ENUM:
	{
	  struct dbr_time_enum  *cval=(struct dbr_time_enum  *) val;
	  arglist=Py_BuildValue("((iiid))", cval->value, cval->severity, 
				cval->status, 
				TS2secs(cval->stamp));
	}
	break;
      case DBR_TIME_FLOAT:
	{
	  struct dbr_time_float  *cval=(struct dbr_time_float  *) val;
	  arglist=Py_BuildValue("((fiid))", 
				cval->value, cval->severity, cval->status, 
				TS2secs(cval->stamp));
	}
	break;
      case DBR_TIME_CHAR:
	{
	  struct dbr_time_char *cval=(struct dbr_time_char *) val;
	  arglist=Py_BuildValue("((biid))", 
				cval->value, cval->severity, cval->status, 
				TS2secs(cval->stamp));
	}
	break;
      case DBR_TIME_LONG:
	{
	  struct dbr_time_long  *cval=(struct dbr_time_long  *) val;
	  arglist=Py_BuildValue("((liid))", 
				cval->value, cval->severity, cval->status, 
				TS2secs(cval->stamp));
	}
	break;
      case DBR_TIME_DOUBLE:
	{
	  struct dbr_time_double  *cval=(struct dbr_time_double  *) val;
	  arglist=Py_BuildValue("((diid))", 
				cval->value, cval->severity, cval->status, 
				TS2secs(cval->stamp));
	}
	break;
      case DBR_CTRL_CHAR:
	{
	  struct dbr_ctrl_char *cval=(struct dbr_ctrl_char *) val;
	  arglist=Py_BuildValue("((biid(sdddddddd)))", 
				cval->value, 
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
	  struct dbr_ctrl_long *cval=(struct dbr_ctrl_long *) val;
	  arglist=Py_BuildValue("((liid(sdddddddd)))", 
				cval->value, 
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
	  struct dbr_ctrl_double *cval=(struct dbr_ctrl_double *) val;
	  arglist=Py_BuildValue("((diid(sddddddddi)))", 
				cval->value, 
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
	  struct dbr_ctrl_enum *cval=(struct dbr_ctrl_enum *) val;
	  char (*strs)[][MAX_ENUM_STRING_SIZE]=(char (*)[][MAX_ENUM_STRING_SIZE]) &cval->strs;
	  unsigned long nstr=cval->no_str, i;
	  PyObject *ptup=PyTuple_New(nstr);

	  for(i=0; i< nstr;i++){
	    PyTuple_SET_ITEM(ptup, i, PyString_FromString((*strs)[i]));
	  }
	  arglist=Py_BuildValue("((iiid(iO)))", 
				cval->value, 
				cval->severity, 
				cval->status, 
				0.0, 
				cval->no_str, 
				ptup
				);
	  Py_XDECREF(ptup);
	}
	break;
      default:
	arglist=Py_BuildValue("((lii))", *((long *) val), -1, status);
      }
    }
    else{/* wave form/array data */
      array=PyList_New(0); /* create Empty python-list object */
      if(array == NULL){
	PyErr_SetString(CaError, "Cannot create list");
	return (PyObject *) NULL;
      }
      switch(type){
      case DBR_STRING:
	MoveDBRtoValue(string, cval->value, dbr_string_t_ptr, "[z]");
	break;
      case DBR_CHAR:
	MoveDBRtoValue(char, &(cval->value), dbr_char_t, "[b]");
	break;
      case DBR_FLOAT:
	MoveDBRtoValue(float, &(cval->value), float , "[f]");
	break;
      case DBR_SHORT:/* same as DBR_TIME_INT */
	MoveDBRtoValue(short, &(cval->value), int, "[i]");
	break;
      case DBR_ENUM:
	MoveDBRtoValue(enum, &(cval->value), int, "[i]");
	break;
      case DBR_LONG:
	MoveDBRtoValue(long, &(cval->value), long, "[l]");
	break;
      case DBR_DOUBLE:
	MoveDBRtoValue(double, &(cval->value), double, "[d]");
	break;
      case DBR_TIME_STRING:
	MoveTimeDBRtoValue(string, cval->value, dbr_string_t_ptr, "[z]");
	break;
      case DBR_TIME_CHAR:
	MoveTimeDBRtoValue(char, &(cval->value), dbr_char_t, "[b]");
	break;
      case DBR_TIME_FLOAT:
	MoveTimeDBRtoValue(float, &(cval->value), float , "[f]");
	break;
      case DBR_TIME_SHORT:/* same as DBR_TIME_INT */
	MoveTimeDBRtoValue(short, &(cval->value), int, "[i]");
	break;
      case DBR_TIME_ENUM:
	MoveTimeDBRtoValue(enum, &(cval->value), int, "[i]");
	break;
      case DBR_TIME_LONG:
	MoveTimeDBRtoValue(long, &(cval->value), long, "[l]");
	break;
      case DBR_TIME_DOUBLE:
	MoveTimeDBRtoValue(double, &(cval->value), double, "[d]");
	break;
      default:
	PyErr_SetString(CaError, "Unkown Data Type");
	Py_XDECREF(array);
	Py_XDECREF(arglist);
	return (PyObject *) NULL;
      }
    }
  }
  //Py_XDECREF(array);
  return arglist;
}
#endif /*_conv_helper.cpp*/

static void exec_callback(struct connection_handler_args args)
{
  PyObject *result=NULL, *arg=NULL;
  /*PyThreadState *cbtstate=NULL;*/
  PyThreadState *oldtstate=NULL;
  PyGILState_STATE gstate;
  _ca_frame *pframe=NULL;
  static struct ca_client_context *last_cac=ca_current_context();

  pframe= (_ca_frame *) ca_puser(args.chid);
  ca_detach_context();
  {
    gstate = PyGILState_Ensure();
    
    if (pframe == NULL){
      PyErr_WarnEx(NULL, "NULL callback frame in exec_callback\n", 1);
      return ;
    }
    
    pframe->lock();
    {
#if defined(DEBUG) && DEBUG
      PyErr_WarnEx(NULL, "Entered in exec_callback\n", 1);
#endif
      if (pframe->pfunc == Py_None){
	PyErr_WarnEx(NULL, "Empty callback frame in exec_callback\n", 1);
      }
      else{
	try{
	  if ((pframe->pfunc != Py_None)){ 
	    if (pframe->tstate) {
#if defined(DEBUG) && DEBUG
	      PyErr_WarnEx(NULL, "restore python thread state in exec-callback\n", 1);
#endif
	      //oldtstate=PyThreadState_Swap(pframe->tstate);
	    }
	    else{
	      oldtstate=NULL;
	      errlogPrintf("Empty thread state to recover in exec-callback\n");
	    }
	    
	    /* call callback routine with arglist -> do whatever it likes */
	    arg=Py_BuildValue("()");
	    Py_XINCREF(pframe->pfunc);
	    
#if defined(DEBUG) && DEBUG
	    PyErr_WarnEx(NULL,  "pfunc before callback:0x%lx (%d)\n", 1);
#endif
	    try{
	      if(PyCallable_Check(pframe->pfunc))
		result=PyObject_CallObject(pframe->pfunc, arg);
	      else
		result=NULL;
	    }
	    catch(...){
	      result=NULL;
	      errlogPrintf("exception while pend_event \n");
	    }
#if defined(DEBUG) && DEBUG
	    PyErr_WarnEx(NULL, "pfunc after callback:0x%lx \n", 1);
#endif
	    Py_XDECREF(result);
	    Py_XDECREF(arg);
	    Py_XDECREF(pframe->pfunc);
	    if (oldtstate) {
	      //cbtstate=PyThreadState_Swap(oldtstate);
#if defined(DEBUG) && DEBUG
	      PyErr_WarnEx(NULL, "thred state recovered \n", 1);
#endif
	    }
	  }
	}
	catch(...){
	  errlogPrintf( "get/put callback un-captured exeption.\n");
	}
      }
    }//lock_ca_frame
    if(pframe->purgeble == 1){
      //PyErr_WarnEx(NULL, "deleting callback frame in get/put callback\n", 1);fflush(stderr);
      delete  pframe;
      pframe=NULL;
    }
    else{
      pframe->unlock();
    }
    PyGILState_Release(gstate);

    ca_attach_context(last_cac);
    //errlogPrintf("back to CA world in exec_callback\n");    
  }

  return;
}

static void con_change_callback(struct connection_handler_args args)
{
  chid ch_id;
  enum channel_state cstat;
  _ca_frame *pframe=NULL;

  // should be called in EPICS/CA context. 

  try{
    ch_id=args.chid;
    cstat=ca_state(ch_id);

    switch(cstat){
    case cs_prev_conn:
      //errlogPrintf("%s is disconnected\n", ca_name(ch_id));
      break;
    case cs_conn:
      //errlogPrintf("%s is connected\n", ca_name(ch_id));
      break;
    case cs_closed:
      //errlogPrintf("%s is closed\n", ca_name(ch_id));
      break;
    case cs_never_conn:
      //errlogPrintf("%s never connected\n", ca_name(ch_id));
      break;
    default:
      //errlogPrintf("%s :this shouldn't occur state = %d\n", ca_name(ch_id), cstat);
      break;
    }
    pframe=(_ca_frame *) ca_puser(args.chid); 
    ////pframe will be locked in exec_callback
    if (pframe != NULL){
      exec_callback(args); /* moved to Python context in exec_callback */
    }
  }
  catch(...){
  }
}

#define MoveValuesToBuffer(DBRBUFTYPE, DBRTYPE, FORMAT) \
      pbuf=calloc(count, sizeof(DBRTYPE));\
      if (pbuf == NULL){\
	PyErr_NoMemory();\
	return NULL;\
      }\
      {\
	DBRTYPE *ptr=(DBRTYPE *) pbuf;\
	DBRBUFTYPE buf;\
        assert(ptr);\
	for(cur=0; cur<count; cur++){\
	  item=PySequence_GetItem(value, cur);\
	  PyArg_Parse(item, FORMAT, &buf);\
	  ptr[cur]=(DBRTYPE) buf;\
	}\
   }	   

static PyObject *Py_ca_put(PyObject *self, PyObject *args)
{
  chid ch_id;
  unsigned int status=-1;
  const char *ca_errmsg=NULL;
  short pyca_field_type=0;
  
  PyObject *value, *myval=Py_None, *item=Py_None, *pcallback=Py_None;
  unsigned long count;
  int retry=0;
  register unsigned long cur=0;
  static size_t const errmsg_size=1024;
  char errmsg[errmsg_size];
  static union  {
    PyObject * O;
    char *str;
    double d;
    long l;
    float f;
    int i;
    short int si;
  } v;

  void *pbuf;
  static  PyThread_type_lock bufmutex=NULL;
  unsigned int lkstatus;

  _ca_frame *pframe;

  //errlogPrintf("ca_put: started\n");fflush(stderr);
  /* now value is always Tuple. Sept. 26, 97 */
  /* extended to */
  pyca_field_type=-1;
  pcallback=Py_None;
  myval=Py_None;
  if(!PyArg_ParseTuple(args, "lO|OOh", 
		       &ch_id, &value, &myval, &pcallback, &pyca_field_type))
    return NULL;
  if(!ch_id){
    PyErr_SetString(CaError, "Null channel ID as an argument");
    return NULL;
  }

  if(PySequence_Check(value) != 1){
    PyErr_SetString(CaError, "ca_put:Second argument must be a tuple");
    return NULL;
  }
  
  count=PyObject_Length(value);
  //errlogPrintf("ca_put: object size %ld\n", (unsigned long) count);
  if(bufmutex == NULL ){
    bufmutex = PyThread_allocate_lock();
  }
  
  Py_BEGIN_ALLOW_THREADS{
    ENTER_CA{
      status = pyCA_Ensure_Connection(ch_id, "Py_ca_put"); 
      /* A function  for channel connection checking */
      if (status == ECA_NORMAL){// should we do this ? NY. 2007/7/04
	while( ca_state(ch_id) == cs_never_conn ) {
	  retry++;
	  try{
	    status=ca_pend_event(0.03); /* wait for ch_id to connect */
	  }
	  catch(...){
	    errlogPrintf("exception while pend_event \n");
	  }
	  if (retry > CA_MAX_RETRY) {
	    status=ECA_TIMEOUT;
	    break; // we don't want return here. We need to recover CA context/Python Thread lock statsus.
	  }
	}
      }
      if (pyca_field_type == -1){
	pyca_field_type=ca_field_type(ch_id);
      }
      ca_errmsg=ca_message(status);
    }LEAVE_CA;
  }Py_END_ALLOW_THREADS;

  if (retry > CA_MAX_RETRY){
    PyErr_SetString(CaError, "Exceed Max retry to connect.");
    return NULL;
  }
  else if(status !=ECA_NORMAL){
    PyErr_SetString(CaError, ca_errmsg);
    return NULL;
  }
  
  if (count == 1){
    //pbuf=calloc(count, sizeof(v)); /* too much memory for some record types */
    pbuf = (void *) &v;
    if ( pbuf == NULL ){
      PyErr_NoMemory();
      return NULL;
    }
#if defined(DEBUG)
    //errlogPrintf("ca_put: buffer allocated at 0x%lx\n", (unsigned long) pbuf);
#endif 
    item=PySequence_GetItem(value, cur);
    Py_BEGIN_ALLOW_THREADS{
      ENTER_CA{
	lkstatus=PyThread_acquire_lock(bufmutex, WAIT_LOCK);
      }LEAVE_CA;
    }Py_END_ALLOW_THREADS;
    switch(pyca_field_type){
    case DBR_STRING:
      if(pbuf){/* free(pbuf);*/}
      PyArg_Parse(item, "z", &pbuf);
      break;
    case DBR_INT: /* DBR_SHORT == DBR_INT */ 
      PyArg_Parse(item, "i", (int *)pbuf); 
      *((short *) pbuf)=*((int *) pbuf); 
      break;
    case DBR_FLOAT:
      PyArg_Parse(item, "f", (float *) pbuf);
      break;
    case DBR_ENUM:
      PyArg_Parse(item, "i", (int *)pbuf);
      *((short *) pbuf)=*((int *) pbuf);
      break;
    case DBR_CHAR:
      PyArg_Parse(item, "b", pbuf);
      break;
    case DBR_LONG:
      PyArg_Parse(item, "l", (long *) pbuf);
      break;
    case DBR_DOUBLE:
      PyArg_Parse(item, "d", (double *) pbuf);
      break;
    default:
      snprintf(errmsg, errmsg_size, "InvalidDataType %u", pyca_field_type);
      PyErr_SetString(CaError, errmsg);
      break;
    } 
    PyThread_release_lock(bufmutex);
    if (pcallback == Py_None ){
      assert(pbuf);
      Py_BEGIN_ALLOW_THREADS{
	lkstatus=PyThread_acquire_lock(bufmutex, WAIT_LOCK);
	ENTER_CA{
	  status=ca_array_put(pyca_field_type, 
			      count, ch_id, pbuf);
	}LEAVE_CA;
      }Py_END_ALLOW_THREADS;
      PyThread_release_lock(bufmutex);
    }
    else{
      pframe=new _ca_frame(NULL, pcallback, Py_None, PyThreadState_Get(), Purgeble);
      {
	assert(pbuf);
	Py_BEGIN_ALLOW_THREADS{
	  lkstatus=PyThread_acquire_lock(bufmutex, WAIT_LOCK);
	  ENTER_CA{
	    status=ca_array_put_callback(pyca_field_type,
					 count, ch_id, pbuf, 
					 get_callback, pframe);
	  }LEAVE_CA;
	}Py_END_ALLOW_THREADS;
	PyThread_release_lock(bufmutex);
      }
      pframe->unlock();
    }
    if(pyca_field_type!= DBR_STRING) {
      //errlogPrintf("freeing put-buffer\n");fflush(stderr);
      if(pbuf != NULL){/*free(pbuf);*/}
      else{ PyErr_WarnEx(NULL, "Null pbuf pointer", 1);}
    }
    Py_XDECREF(item);
    if(status != ECA_NORMAL) {
      PyErr_SetString(CaError, ca_message(status));
      return NULL;
    }
    return Py_BuildValue("i", status);
  }
  else if (count > 1) { /* array records */
    switch(pyca_field_type){
    case DBR_STRING:
      pbuf=calloc(count, sizeof(dbr_string_t));
      //errlogPrintf("ca_put: string buffer allocated at 0x%lx\n", (unsigned long) pbuf);
      if (pbuf == NULL){
	PyErr_NoMemory();
	return NULL;
      }
      {
	register dbr_string_t *ptr=(dbr_string_t *) pbuf;
	char *str;
	for(cur=0;cur<count;cur++){
	  item=PySequence_GetItem(value, cur);
	  PyArg_Parse(item, "z", &str);
	  strncpy((char *)&(ptr[cur]), 
		  str, sizeof(dbr_string_t));
	}
      }
      break;
    case DBR_INT:/* note that MoveValuesToBuffer is Macro */
      MoveValuesToBuffer(dbr_long_t, dbr_int_t, "i"); 
      break;
    case DBR_FLOAT:
      MoveValuesToBuffer(dbr_double_t, dbr_float_t, "d");
      break;
    case DBR_ENUM:
      MoveValuesToBuffer(dbr_long_t, dbr_enum_t, "l");
      break;
    case DBR_CHAR:
      MoveValuesToBuffer(dbr_char_t, dbr_char_t, "b");
      break;
    case DBR_LONG:
      MoveValuesToBuffer(dbr_long_t, dbr_long_t, "l");
      break;
    case DBR_DOUBLE:
      MoveValuesToBuffer(dbr_double_t, dbr_double_t, "d");
      break;
    default:
      PyErr_SetString(CaError, "Invalid field type");
      if(item != NULL) { Py_DECREF(item);}
      return NULL;
    }
    if (pcallback == Py_None ){
      Py_BEGIN_ALLOW_THREADS{
	lkstatus=PyThread_acquire_lock(bufmutex, WAIT_LOCK);
	ENTER_CA{
	  status=ca_array_put(pyca_field_type, 
			      count, ch_id, pbuf);
	}LEAVE_CA;
      }Py_END_ALLOW_THREADS;
      PyThread_release_lock(bufmutex);
    }
    else{
      pframe=new _ca_frame(NULL, pcallback, Py_None, PyThreadState_Get(), Purgeble);
      Py_BEGIN_ALLOW_THREADS{
	ENTER_CA{
	  status=ca_array_put_callback(pyca_field_type,
				       count, ch_id, pbuf, 
				       get_callback, pframe);
	}LEAVE_CA;
      }Py_END_ALLOW_THREADS;
      pframe->unlock();
    }
    if(!pbuf) {free(pbuf);}
    //else{ PyErr_SetString(CaError, "Null  pbuf pointer");}
    Py_DECREF(item); item=NULL;
    if (status != ECA_NORMAL){
      Py_BEGIN_ALLOW_THREADS{
	ENTER_CA{
	  snprintf(errmsg, errmsg_size, ca_message(status));
	}LEAVE_CA;
      }Py_END_ALLOW_THREADS;
      PyErr_SetString(CaError, errmsg);
      return NULL;
    }
    return Py_BuildValue("i", status);
  }
  else{
    snprintf(errmsg, errmsg_size, "InvalidDataType %u", pyca_field_type);
    PyErr_SetString(CaError, errmsg);
    return NULL;
  }
}

static PyObject *
Py_ca_monitor(PyObject *self, PyObject *args){
  chid ch_id;
  int status=-1;
  unsigned long ecount=0;
  unsigned long evmask=DBE_VALUE|DBE_ALARM;
  chtype type;
  PyObject *pcallback, *rval;
  _ca_frame *pframe;
  const char *ca_errmsg=NULL;

  if(!PyArg_ParseTuple(args, "lOl|l", &ch_id, &pcallback, &ecount, &evmask))
    return NULL;

  Py_BEGIN_ALLOW_THREADS{
    ENTER_CA{
      status=pyCA_Ensure_Connection(ch_id, "Py_ca_monitor"); 
      /* MACRO for channel connection checking */
      ca_errmsg=ca_message(status);
    }LEAVE_CA;
  }Py_END_ALLOW_THREADS;

  if (status !=ECA_NORMAL){
    PyErr_SetString(CaError, ca_errmsg);
    return NULL;
  }

  pframe=new _ca_frame(NULL, pcallback, Py_None, PyThreadState_Get(), DontPurge);
  {
    Py_BEGIN_ALLOW_THREADS{
      ENTER_CA{
	type =dbf_type_to_DBR_TIME(ca_field_type(ch_id));
	if (ecount == 0){
	  ecount=ca_element_count(ch_id);
	}
	status=ca_create_subscription(type, ecount, ch_id, evmask, 
				      get_callback, pframe, 
				      &pframe->EVID);
      }LEAVE_CA;
    } Py_END_ALLOW_THREADS;
    SEVCHK(status, "Py_monitor");
    rval=PyCObject_FromVoidPtr(pframe, NULL);
  }
  pframe->unlock();// frame is locked at creation
  return rval;
}

static PyObject *
Py_ca_clear_monitor(PyObject *self, PyObject *args){
  int status=-1;
  PyObject *obj;
  _ca_frame *pframe=NULL;
  
  if(!PyArg_ParseTuple(args, "O", &obj)){
    PyErr_SetString(CaError, "Invalid argument");
    return NULL;
  }
  
  pframe = (_ca_frame *) PyCObject_AsVoidPtr(obj);
  if ( pframe== NULL){
    PyErr_SetString(CaError, "NULL callback frame to clear.");
    return NULL;
  }
  pframe->lock();
  {
    if(pframe->EVID == NULL){
      PyErr_SetString(CaError, "Null EVENT ID as an argument");
      pframe->unlock();
      return NULL;
    }
    Py_BEGIN_ALLOW_THREADS{
      ENTER_CA{
	status=ca_clear_subscription(pframe->EVID);
      }LEAVE_CA;
    }Py_END_ALLOW_THREADS;
    SEVCHK(status, "Py_Clear_monitor");
  }
  delete  pframe;pframe=NULL;
  return Py_BuildValue("i", status);
}

static PyObject *
Py_ca_pend_io(PyObject *self, PyObject *args){

  ca_real wait;
  int status=0 ;

  if(!PyArg_ParseTuple(args, "d", &wait))
    return NULL;

  Py_BEGIN_ALLOW_THREADS{
    ENTER_CA{
#if 0
      errlogPrintf( ">> pend_io\n");
      fflush(stderr);
#endif
      try{
	status=ca_pend_io(wait);
      }
      catch(...){
	status=-1;
	errlogPrintf( "exception in pend_io\n");
      }
#if 0
      errlogPrintf( "<<done pend_io tmo:%f, status:%d\n", wait, status);
      fflush(stderr);
#endif
    }LEAVE_CA;
  }Py_END_ALLOW_THREADS;

  SEVCHK(status, "py_pend_io");
  return Py_BuildValue("i", CA_EXTRACT_MSG_NO(status));
} 

static PyObject *
Py_ca_test_io(PyObject *self, PyObject *args){

  int status =0;
  int msgno=0;

  Py_BEGIN_ALLOW_THREADS{
    ENTER_CA{
      status=ca_test_io();
      msgno=CA_EXTRACT_MSG_NO(status);
      SEVCHK(status, "py_test_io");
    }LEAVE_CA;
  }Py_END_ALLOW_THREADS;

  return Py_BuildValue("i", msgno);
} 

static PyObject *
Py_ca_pend_event(PyObject *self, PyObject *args){
  ca_real wait;
  double dwait;
  int status=-1;
  int msgno=0;

  if(!PyArg_ParseTuple(args, "d", &dwait))
    return NULL;

  wait=dwait;

  Py_BEGIN_ALLOW_THREADS{
    ENTER_CA{
      try{
	status=ca_pend_event(wait);
      }
      catch(...){
	status=-1;
	errlogPrintf("exception while pend_event \n");
      }
      msgno=CA_EXTRACT_MSG_NO(status);
    }LEAVE_CA;
  }Py_END_ALLOW_THREADS;

  return Py_BuildValue("i", msgno);
}

static PyObject *
Py_ca_poll(PyObject *self, PyObject *args){

  int status ;
  int msgno=0;
   
  Py_BEGIN_ALLOW_THREADS{
    ENTER_CA{
      status=ca_poll();
      msgno=CA_EXTRACT_MSG_NO(status);
    }LEAVE_CA;
  }Py_END_ALLOW_THREADS;
 
  return Py_BuildValue("i", msgno);
}

static PyObject *
Py_ca_flush(PyObject *self, PyObject *args){
  int status;
  int msgno=0;

  Py_BEGIN_ALLOW_THREADS{
    ENTER_CA{
      status=ca_flush_io();
      msgno=CA_EXTRACT_MSG_NO(status);
    }LEAVE_CA;
  }Py_END_ALLOW_THREADS;

  return Py_BuildValue("i", msgno);
} 

static PyObject *
Py_ca_status(PyObject *self, PyObject *args){
  chid ch_id;
  int status=-1;

  if(!PyArg_ParseTuple(args, "l", &ch_id))
    return NULL;
  if(!ch_id){
    PyErr_SetString(CaError, "Null channel ID as an argument");
    return NULL;
  }

  Py_BEGIN_ALLOW_THREADS{
    ENTER_CA{
      status=ca_state(ch_id) - cs_conn;
    }LEAVE_CA;
  }Py_END_ALLOW_THREADS;

  return Py_BuildValue("i", status);
} 

static PyObject *Py_ca_channelInfo(PyObject *self, PyObject *args){
  chid ch_id;
  PyObject *pobj;
  //  const char *ca_errmsg;
  short pyca_field_type=0;
  size_t pyca_element_count=0;
  void *pyca_puser=NULL;
  enum channel_state pyca_state=cs_never_conn;
  unsigned pyca_read_access=0, pyca_write_access=0;

  const size_t blen=1024;
  char pbuf[blen];

  if(!PyArg_ParseTuple(args, "l", &ch_id))
    return NULL;

  if(!ch_id){
    PyErr_SetString(CaError, "Null channel ID as an argument");
    return NULL;
  }

  Py_BEGIN_ALLOW_THREADS{
    ENTER_CA{
      pyca_field_type=ca_field_type(ch_id);
      pyca_element_count=ca_element_count(ch_id);
      pyca_puser=ca_puser(ch_id);
      pyca_state=ca_state(ch_id);
      pyca_read_access=ca_read_access(ch_id);
      pyca_write_access=ca_write_access(ch_id);
      ca_get_host_name(ch_id, pbuf, blen);
    }LEAVE_CA;
  }Py_END_ALLOW_THREADS;

  pobj = Py_BuildValue(
		       "(illisii)", 
		       pyca_field_type, 
		       pyca_element_count, 
		       pyca_puser, 
		       pyca_state, 
		       pbuf, 
		       pyca_read_access, 
		       pyca_write_access
		       );
  //Py_XINCREF(pobj);
  return pobj;
} 

static PyObject *Py_ca_name(PyObject *self, PyObject *args){
     chid ch_id;
     PyObject *pobj=NULL;
 
     if(!PyArg_ParseTuple(args, "l", &ch_id))
         return NULL;
     if(!ch_id){
         PyErr_SetString(CaError, "Null channel ID as an argument");
         return NULL;
     }
     ENTER_CA{
        pobj = PyString_FromString(ca_name(ch_id));
    }LEAVE_CA;
 
     Py_XINCREF(pobj);
     return pobj;
 }
 
 static PyObject *Py_dbf_text(PyObject *self, PyObject *args){
     chtype field_type;
     PyObject *pobj=NULL;
 
     if(!PyArg_ParseTuple(args, "l", &field_type))
         return NULL;
     ENTER_CA{
         pobj = PyString_FromString(dbf_type_to_text(field_type));
     }LEAVE_CA;
 
     Py_XINCREF(pobj);
     return pobj;
 }
 
 static PyObject *Py_dbr_text(PyObject *self, PyObject *args){
     chtype req_type;
     PyObject *pobj=NULL;
 
     if(!PyArg_ParseTuple(args, "l", &req_type))
         return NULL;
     ENTER_CA{
         pobj = PyString_FromString(dbr_type_to_text(req_type));
     }LEAVE_CA;
 
     Py_XINCREF(pobj);
     return pobj;
 }
 
#define ca_sock_id(CHID)  ca_sock_id_function(CHID)

#if (EPICS_VERSION >=3) && (EPICS_REVISION >=14)

static int ca_sock_id_function(chid CHID){
  errlogPrintf( "you cannot get sock id diretly. Use fd_register function , instead");
  return -1;
}
#endif

static PyObject *Py_ca_fileno(PyObject *self, PyObject *args){
  chid ch_id;
  int sock;

  if(!PyArg_ParseTuple(args, "l", &ch_id))
    return NULL;
  if(!ch_id){
    PyErr_SetString(CaError, "Null channel ID as an argument");
    return NULL;
  }
  sock=ca_sock_id(ch_id);
  return Py_BuildValue(
		    "i", 
		    sock);
} 

static void Py_fd_register_wrapper(void *args, int fd, int condition)
{
  PyObject *func, *arg;
  _ca_frame *pframe=NULL;
  PyObject *result, *uargs;
  PyThreadState *cbtstate=NULL;
  PyGILState_STATE gstate;

  {
    static struct ca_client_context *last_cac=ca_current_context();

    ca_detach_context();
    gstate = PyGILState_Ensure();
    
    //errlogPrintf("fd_register_wrapper");	   
    pframe=(_ca_frame *) args;
    if (pframe == NULL) {
      PyErr_WarnEx(NULL, "No callable object", 1);
      return;
    }
    pframe->lock();
    {
      func=pframe->pfunc;
      arg= pframe->args;
      cbtstate=pframe->tstate;
      
      {
	if (PyCallable_Check(func)){
	  uargs=Py_BuildValue("(Oii)", arg, fd, condition);
	  Py_XINCREF(func); Py_XINCREF(uargs);Py_XINCREF(arg);
	  try{
	    result=PyObject_CallObject(func, uargs);
	  }
	  catch(...){
	    PyErr_WarnEx(NULL, "exception while fd reigster\n", 1);
	  }
	  Py_XDECREF(func);Py_XDECREF(uargs);Py_XDECREF(arg);
	}
      }
    }
    pframe->unlock();
    PyGILState_Release(gstate);
    ca_attach_context(last_cac);
  }
  return;
}

static PyObject *Py_add_fd_registration(PyObject *self, PyObject *args){
  int status;
  PyObject *func, *arg;
  _ca_frame *pframe;

  if(!PyArg_ParseTuple(args, "OO", &func, &arg))
    return NULL;

  if(!args){
    PyErr_SetString(CaError, "Null channel ID as an argument");
    return NULL;
  }
  pframe=new _ca_frame(NULL, func, arg, PyThreadState_Get(), DontPurge);
  Py_BEGIN_ALLOW_THREADS{
    ENTER_CA{
      status=ca_add_fd_registration(Py_fd_register_wrapper, pframe);
      SEVCHK(status, "add_fd_regstration: Fd registration failed.\n");
    }LEAVE_CA;
  }Py_END_ALLOW_THREADS;
  pframe->unlock();
  Py_INCREF(Py_None);
  return Py_None;
} 


/************************ sync. group routines ****************************/
static PyObject *
Py_sg_create(PyObject *self, PyObject *args){

  int status=-1;
  CA_SYNC_GID gid;

  Py_BEGIN_ALLOW_THREADS{
    ENTER_CA{
      status=ca_sg_create(&gid);
      SEVCHK(status, "Py_sg_create");
    }LEAVE_CA;
  }Py_END_ALLOW_THREADS;

  if (status != ECA_NORMAL){
    PyErr_SetString(CaError, ca_message(status));
    return NULL;
  }
  else {
    return Py_BuildValue("l", gid);
  }
}

static PyObject *
Py_sg_delete(PyObject *self, PyObject *args){

  int status=-1;

  CA_SYNC_GID gid;

  if(!PyArg_ParseTuple(args, "l", &gid)){
    return NULL;
  }
  if(gid < 0){
    PyErr_SetString(CaError, "Null sync. Group ID as an argument");
    return NULL;
  }

  Py_BEGIN_ALLOW_THREADS{
    ENTER_CA{
      status=ca_sg_delete(gid);
      SEVCHK(status, "Py_sg_delete")
    }LEAVE_CA;
  }Py_END_ALLOW_THREADS;

  if (status != ECA_NORMAL){
    PyErr_SetString(CaError, ca_message(status));
    return NULL;
  }
  else {
    return Py_BuildValue("l", gid);
  }
}

static PyObject *
Py_sg_block(PyObject *self, PyObject *args){

  int status=-1;
  CA_SYNC_GID gid;
  float tmo;

  if(!PyArg_ParseTuple(args, "lf", &gid, &tmo)){
    return NULL;
  }
  if(gid < 0){
    PyErr_SetString(CaError, "Null sync. Group ID as an argument");
    return NULL;
  }
  Py_BEGIN_ALLOW_THREADS{
    ENTER_CA{
      status=ca_sg_block(gid, ( (tmo > 0.0) ? tmo : 1.0));
    }LEAVE_CA;
  }Py_END_ALLOW_THREADS;

  switch(status){
  case ECA_NORMAL:
    return Py_BuildValue("i", 0);
    break;
  case ECA_TIMEOUT:
  case ECA_EVDISALLOW:
  case ECA_BADSYNCGRP:
  default:
    PyErr_SetString(CaError, ca_message(status));
    return NULL;
  }
}

static PyObject *
Py_sg_test(PyObject *self, PyObject *args){

  int status=-1;
  CA_SYNC_GID gid;

  if(!PyArg_ParseTuple(args, "l", &gid)){
    return NULL;
  }
  if(gid < 0){
    PyErr_SetString(CaError, "Null sync. Group ID as an argument");
    return NULL;
  }

  Py_BEGIN_ALLOW_THREADS{
    ENTER_CA{
      status=ca_sg_test(gid);
      SEVCHK(status, "Py_sg_create");
    }LEAVE_CA;
  }Py_END_ALLOW_THREADS;

  switch(status){
  case ECA_IODONE:
    return Py_BuildValue("i", 0);
    break;
  case ECA_IOINPROGRESS:
    return Py_BuildValue("i", -1);
    break;
  default:
    PyErr_SetString(CaError, ca_message(status));
    return NULL;
  }
}

static PyObject *
Py_sg_reset(PyObject *self, PyObject *args){

  int status=-1;
  CA_SYNC_GID gid;

  if(!PyArg_ParseTuple(args, "l", &gid)){
    return NULL;
  }

  if(gid < 0){
    PyErr_SetString(CaError, "Null Sync. Group ID as an argument");
    return NULL;
  }

  ENTER_CA{
    Py_BEGIN_ALLOW_THREADS{
	status=ca_sg_reset(gid);
	SEVCHK(status, "Py_sg_create");
    }Py_END_ALLOW_THREADS;
  }LEAVE_CA;


  if (status != ECA_NORMAL){
    PyErr_SetString(CaError, ca_message(status));
    return NULL;
  }
  else {
    return Py_BuildValue("i", CA_EXTRACT_MSG_NO(status));
  }
}

static PyObject *
Py_sg_put(PyObject *self, PyObject *args){
  unsigned long count;
  int status=-1;
  CA_SYNC_GID gid;
  chid ch_id;
  const char *ca_errmsg=NULL;
  short pyca_field_type=-1;
  unsigned long  pyca_element_count=0;

  size_t size;
  PyObject *value, *rval;
  chtype type;
  void *pbuf=NULL;

  if(!PyArg_ParseTuple(args, "illOl", &gid, &ch_id, &pbuf, &value, &type)){
    return NULL;
  }

  if(gid < 0){
    PyErr_SetString(CaError, "Null sync group ID as an argument");
    return NULL;
  }
  if( ! PySequence_Check(value) ){
    Py_XDECREF(value);
    return NULL;
  }

  Py_BEGIN_ALLOW_THREADS{
    ENTER_CA{
      status=pyCA_Ensure_Connection(ch_id, "Py_ca_monitor"); /* MACRO for channel connection checking */
      ca_errmsg=ca_message(status);
      if(status == ECA_NORMAL){
	pyca_field_type=ca_field_type(ch_id);
	pyca_element_count=ca_element_count(ch_id);
      }
    }LEAVE_CA;
  }Py_END_ALLOW_THREADS;

  if (status !=ECA_NORMAL){
    PyErr_SetString(CaError, ca_errmsg);
    return NULL;
  }

  if (type < 0){
    type=dbf_type_to_DBR(pyca_field_type);
  }

  if(!pbuf){
    Py_BEGIN_ALLOW_THREADS{
      ENTER_CA{
	count=pyca_element_count;
	size=dbr_size_n(count, dbf_type_to_DBR(pyca_field_type))
	  +dbr_size_n(1, dbf_type_to_DBR_TIME(pyca_field_type)) ;
	pbuf=calloc(1, size);
	errlogPrintf("py_sg_put: allocate memory at 0x%p\n",pbuf);
      }LEAVE_CA;
    }Py_END_ALLOW_THREADS;
    if(!pbuf){
      PyErr_NoMemory();
      return NULL;
    }
  }
#if defined(DEBUG) && DEBUG
  else{
    errlogPrintf("py_sg_get: use memory at 0x%p size %ld for %ld\n",pbuf,size,count);
  }
#endif
  
  count=PyObject_Length(value); /* just ignore data longer than channel data type */
  count=((count < pyca_element_count)?count:pyca_element_count);
  if (count < 0){
    PyErr_SetString(CaError, "invalid data length");
    return NULL;
  }

  status=_convert_Py_to_ca(type, count, 
			   value, pbuf);
  if(status != 0){
    return NULL;
  }
  Py_BEGIN_ALLOW_THREADS{
    ENTER_CA{
      status=ca_sg_array_put(gid, type, count, ch_id, pbuf );
      SEVCHK(status, "Py_sg_array_put");
    }LEAVE_CA;
  }Py_END_ALLOW_THREADS;

  if (status != ECA_NORMAL){
    PyErr_SetString(CaError, ca_message(status));
    return NULL;
  }
  else {
    rval=Py_BuildValue("l",  pbuf);
    return rval;
  }
}

static PyObject *
Py_sg_get(PyObject *self, PyObject *args){

  int status=0, retry=0;
  CA_SYNC_GID gid;
  long type, count;
  chid ch_id;
  const char *ca_errmsg=NULL;
  unsigned long pyca_element_count=0;
  short pyca_field_type=0;

  void *pbuf=NULL;
  size_t size;
  PyObject *rval;

  if(!PyArg_ParseTuple(args, "ill", &gid, &ch_id, &pbuf)){
    return NULL;
  }

  Py_BEGIN_ALLOW_THREADS{
    ENTER_CA{
      status=pyCA_Ensure_Connection(ch_id, "Py_sg_get"); /* A function for channel connection checking */
      ca_errmsg=ca_message(status);
      if (status == ECA_NORMAL){
	while( ca_state(ch_id) != cs_conn ) {
	  retry++;
	  try{
	    ca_pend_event(0.02); /* wait for ch_id to connect */
	  }
	  catch(...){
	    errlogPrintf("exception while pend_event \n");
	  }
	  if (retry > CA_MAX_RETRY){
	    status=ECA_TIMEOUT;
	    ca_errmsg=ca_message(status);
	    break;
	  }
	}
      }
      if (status == ECA_NORMAL){
	pyca_element_count=ca_element_count(ch_id);
	pyca_field_type=ca_field_type(ch_id);
      }
    }LEAVE_CA;
  }Py_END_ALLOW_THREADS;

  if(status != ECA_NORMAL){
    PyErr_SetString(CaError, ca_errmsg);
    return NULL;
  }
  
  type=dbf_type_to_DBR_TIME(pyca_field_type);
  count=pyca_element_count;
  if(!pbuf){
    Py_BEGIN_ALLOW_THREADS{
      ENTER_CA{
	size=dbr_size_n(count, dbf_type_to_DBR(pyca_field_type))
	  +dbr_size_n(1, dbf_type_to_DBR_TIME(pyca_field_type)) ;
	pbuf=calloc(1, size);
#if defined(DEBUG) && DEBUG
	errlogPrintf("py_sg_get: allocate memory at 0x%p size %ld for %ld\n",pbuf,size,count);
#endif
      }LEAVE_CA;
    }Py_END_ALLOW_THREADS;
    if(!pbuf){
      PyErr_NoMemory();
      return NULL;
    }
  }
#if defined(DEBUG) && DEBUG
  else{
     errlogPrintf("py_sg_get: use memory at 0x%p size %ld for %ld\n",pbuf,size,count);
  }
#endif

  Py_BEGIN_ALLOW_THREADS{
    ENTER_CA{
      status=ca_sg_array_get(gid, type, count, ch_id, pbuf);
      SEVCHK(status, "Py_sg_array_get");
      if (status != ECA_NORMAL){
	ca_errmsg=ca_message(status);
      }
    }LEAVE_CA;
  }Py_END_ALLOW_THREADS;

  if (status != ECA_NORMAL){
    PyErr_SetString(CaError, ca_errmsg);
    return NULL;
  }
  else {
    rval=Py_BuildValue("l",  pbuf);
    return rval;
  }
}

static PyObject *
Py_ca_convert(PyObject *self, PyObject *args){
  
  int status=-1;
  chid ch_id;
  void *pbuf;
  PyObject *pobj;

  if(!PyArg_ParseTuple(args, "ll", &ch_id, &pbuf)){
    return NULL;
  }

  if(!ch_id  || !pbuf){
    PyErr_SetString(CaError, "Invalid Argument value(s)");
    return NULL;
  }

  pobj=_convert_ca_to_Python(dbf_type_to_DBR_TIME(ca_field_type(ch_id)), 
			     ca_element_count(ch_id), 
			     pbuf, status);

  if(pobj){
    return pobj;
  }
  else{
    PyErr_SetString(CaError, "Invalid CA Values");
    return NULL;
  }
}

#define SgMoveValuesToBuffer(DBRBUFTYPE, DBRTYPE, FORMAT) \
      {\
         DBRTYPE *ptr=(DBRTYPE *) pbuf;\
	 DBRBUFTYPE buf;\
	 unsigned long cur;\
	 for(cur=0;cur<count;cur++){\
           item=PySequence_GetItem(value, cur);\
           PyArg_Parse(item, FORMAT, &buf);\
           Py_XDECREF(item); \
           ptr[cur]=(DBRTYPE) buf;\
       }}

static int
_convert_Py_to_ca(chtype type, unsigned long count, PyObject *value, void *pbuf){
  PyObject *item;
  unsigned long cur;

  if (pbuf ==(void *) NULL){
    return -1;
  }
  switch(type){
  case DBR_STRING:
    {
      register dbr_string_t *pstr=(dbr_string_t *) pbuf;
      char *str;
      for (cur=0; cur< count; cur++){
	item=PySequence_GetItem(value, cur);
	PyArg_Parse(item, "z", &str);
	Py_XDECREF(item);
	strncpy((char *)&(pstr[cur]), str, sizeof(dbr_string_t));
      }
    }
    break;
  case DBR_INT:/* note that SgMoveValuesToBuffer is Macro */
    SgMoveValuesToBuffer(dbr_long_t, dbr_int_t, "i"); 
    break;
  case DBR_FLOAT:
    SgMoveValuesToBuffer(dbr_double_t, dbr_float_t, "d");
    break;
  case DBR_ENUM:
    SgMoveValuesToBuffer(dbr_long_t, dbr_enum_t, "l");
    break;
  case DBR_CHAR:
    SgMoveValuesToBuffer(dbr_char_t, dbr_char_t, "b");
    break;
  case DBR_LONG:
    SgMoveValuesToBuffer(dbr_long_t, dbr_long_t, "l");
    break;
  case DBR_DOUBLE:
    SgMoveValuesToBuffer(dbr_double_t, dbr_double_t, "d");
    break;
  default:
    PyErr_SetString(CaError, "Invalid field type");
    return -1;
  }
  return 0;
}

/******************** end of sync. group routines **************************/
static PyObject *
Py_ca_v42_ok(PyObject *self, PyObject *args){
  chid ch_id;
  int v42_ok=0;

  if(!PyArg_ParseTuple(args, "l", &ch_id))
    return NULL;
  if(!ch_id){
    PyErr_SetString(CaError, "Null channel ID as an argument");
    return NULL;
  }

  Py_BEGIN_ALLOW_THREADS{
    ENTER_CA{
      v42_ok=ca_v42_ok(ch_id) ;
    }LEAVE_CA;
  }Py_END_ALLOW_THREADS;

  return Py_BuildValue("i", v42_ok);
} 

static PyObject *
Py_ca_version(PyObject *self, PyObject *args){
  const char *version_string=NULL;

  Py_BEGIN_ALLOW_THREADS{
    ENTER_CA{
      version_string=ca_version() ;
    }LEAVE_CA;
  }Py_END_ALLOW_THREADS;

  return Py_BuildValue("s", version_string);
} 


/*************************************************************************/
typedef void (*signal_handler)(int);

/* alternative exception handler for EPICS/CA */
static void exceptionCallback(struct exception_handler_args args)
{

  /* check ca_contex */
  if ( ca_current_context() == NULL){
    if (my_cac) {
      if(DEBUG)
	errlogPrintf("NULL ca_context in exception callback"
		     "->switched to my ca-context.\n");
      ca_attach_context(my_cac);
    }
    else{
      errlogPrintf("Null ca_context!\n");
    }
  }
  else if(ca_current_context()!= my_cac){
    errlogPrintf("ca_context:different from my ca-context.\n");    
  }

  if ((args.chid !=NULL) && (args.op != CA_OP_OTHER)) {
    exceptionCallbackFormated(
			      args.stat, 
			      args.pFile, 
			      args.lineNo, 
			      "%s - with request chan=%s op=%ld data type=%s count=%ld", 
			      args.ctx, 
			      ca_name (args.chid), 
			      args.op, 
			      dbr_type_to_text(args.type), 
			      args.count);
  }
  else {
    exceptionCallbackFormated(
			      args.stat, 
			      args.pFile, 
			      args.lineNo, 
			      NULL, 
			      args.ctx);
  }
}

static int exceptionCallbackFormated(long ca_status, const char *pfilenm, 
				 int lineno, const char *pFormat, 
				 ...)
{
  va_list             theArgs;
  static const char   *severity[] =
  { "Warning", 
    "Success", 
    "Error", 
    "Info", 
    "Fatal", 
    "Fatal", 
    "Fatal", 
    "Fatal"
  };
  
  va_start (theArgs, pFormat);
  errlogPrintf ("CA.Client.Diagnostic.................................\n");

  errlogPrintf ("    %s: \"%s\"\n", 
		severity[CA_EXTRACT_SEVERITY(ca_status)], 
		ca_message (ca_status));

  if  (pFormat) {
    errlogPrintf ("    Context: \"");
    errlogVprintf (pFormat, theArgs);
    errlogPrintf ("\"\n");
  }
  
  if (pfilenm) {
    errlogPrintf("Source File: %s Line Number: %d\n", 
		 pfilenm, 
		 lineno);
  }
  
  errlogPrintf(".....................................................\n");
  va_end (theArgs);
  
  return 0;
}

/* for Tk event loop */


#if defined(HAVE_CREATEFILEHANDLER)
static void Tk_ca_fd_register(void *pfdctx, int fd, int condition)
{
  if(condition){
    Tk_CreateFileHandler(fd, TK_READABLE, Tk_ca_service, pfdctx);
  }
  else{
    Tk_DeleteFileHandler(fd);
  }
}
/*
******************************************************
* NAME
*	Tk_ca_service()
* DESCRIPTION
*	1) call ca_pend_event to allow event handler execution
******************************************************
* Note: Feb. 2, 2000 by NY
* This function will be called from Tcl interpreter, which
* set tcl_lock and has NULL Python-thread state.
*/
static void Tk_ca_service(void *pt, int i)
{
  struct timeval tp, tp1;
  struct timezone tzp;
  gettimeofday(&tp1, &tzp);

  if (DEBUG){
    fprintf(stderr, "in ca_service ... ");fflush(stderr);
  }
  ENTER_CA{
    ca_poll();
  }LEAVE_CA;
  if(DEBUG) {
    fprintf(stderr, "done!\n");fflush(stderr);	  
    gettimeofday(&tp1, &tzp);
    fprintf(stderr, " %d %d\n", (int) tp.tv_sec, (int) tp.tv_usec);
    fprintf(stderr, " %d %d\n", (int) tp1.tv_sec, (int) tp1.tv_usec);
  }
}

static PyObject *Py_ca_register_to_Tk(PyObject *self, PyObject *args)
{

  Py_BEGIN_ALLOW_THREADS{
    ENTER_CA{
      int status;
      status=ca_add_fd_registration(Tk_ca_fd_register, NULL);
    }LEAVE_CA;
  }Py_END_ALLOW_THREADS;

  Py_INCREF(Py_None);
  return Py_None;
}
#endif /*defined(HAVE_CREATEFILEHANDLER)*/
