import os
import warnings

if os.environ.get('CACHANNEL_BACKEND') == 'caffi':
    from caffi.ca import *
    from caffi.macros import *
    from caffi.constants import *
else:
    try:
        from ._ca import *
    except:
        warnings.warn("c extension is not available, trying caffi as fallback", RuntimeWarning)
        from caffi.ca import *
        from caffi.macros import *
        from caffi.constants import *

