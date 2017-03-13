"""
This module provides support for the EPICS motor record.

Author:         Mark Rivers

Created:        Sept. 16, 2002

Modifications:

- Mar. 7, 2017 Xiaoqiang Wang

  - Reformat the docstring and code indent.
  - Use class property to expose certain fields.
"""
import time

import epicsPV


class epicsMotor:
    """
    This module provides a class library for the EPICS motor record.
    It uses the :class:`epicsPV.epicsPV` class, which is in turn a subclass of :class:`CaChannel.CaChannel`

    Certain motor record fields are exposed as class properties.
    They can be both read and written unless otherwise noted.

    ===============  ===========================  =====
    Property         Description                  Field
    ===============  ===========================  =====
    slew_speed       Slew speed or velocity       .VELO
    base_speed       Base or starting speed       .VBAS
    acceleration     Acceleration time (sec)      .ACCL
    description      Description of motor         .DESC
    resolution       Resolution (units/step)      .MRES
    high_limit       High soft limit (user)       .HLM
    low_limit        Low soft limit (user)        .LLM
    dial_high_limit  High soft limit (dial)       .DHLM
    dial_low_limit   Low soft limit (dial)        .DLLM
    backlash         Backlash distance            .BDST
    offset           Offset from dial to user     .OFF
    done_moving      1=Done, 0=Moving, read-only  .DMOV
    ===============  ===========================  =====

    >>> m = epicsMotor('13BMD:m38')
    >>> m.move(10)              # Move to position 10 in user coordinates
    >>> m.wait()                # Wait for motor to stop moving
    >>> m.move(50, dial=True)   # Move to position 50 in dial coordinates
    >>> m.wait()                # Wait for motor to stop moving
    >>> m.move(1, step=True, relative=True) # Move 1 step relative to current position
    >>> m.wait(start=True, stop=True) # Wait for motor to start, then to stop
    >>> m.stop()                # Stop moving immediately
    >>> high = m.high_limit     # Get the high soft limit in user coordinates
    >>> m.dial_high_limit = 100 # Set the high limit to 100 in dial coodinates
    >>> speed = m.slew_speed    # Get the slew speed
    >>> m.acceleration = 0.1    # Set the acceleration to 0.1 seconds
    >>> val = m.get_position()      # Get the desired motor position in user coordinates
    >>> dval = m.get_position(dial=True)# Get the desired motor position in dial coordinates
    >>> rbv = m.get_position(readback=True) # Get the actual position in user coordinates
    >>> rrbv = m.get_position(readback=True, step=True) # Get the actual motor position in steps
    >>> m.set_position(100)   # Set the current position to 100 in user coordinates
    >>> m.set_position(10000, step=True) # Set the current position to 10000 steps
    """

    class PVProperty(object):
        def __init__(self, name, readonly=False):
            self.name = name
            self.readonly = readonly

        def __get__(self, instance, owner):
            if instance is None:
                return self
            return instance.pvs[self.name].getw()

        def __set__(self, instance, value):
            if instance is None or self.readonly:
                return
            instance.pvs[self.name].putw(value)

    slew_speed = PVProperty('velo')
    base_speed = PVProperty('vbas')
    acceleration = PVProperty('accl')
    description = PVProperty('desc')
    resolution = PVProperty('mres')
    high_limit = PVProperty('hlm')
    low_limit = PVProperty('llm')
    dial_high_limit = PVProperty('dhlm')
    dial_low_limit = PVProperty('dllm')
    backlash = PVProperty('bdst')
    offset = PVProperty('off')
    done_moving = PVProperty('dmov', readonly=True)

    def __init__(self, name):
        """
        Creates a new epicsMotor instance.

        :param str name: The name of the EPICS motor record without any trailing period or field name.

        >>> m = epicsMotor('13BMD:m38')
        """
        self.pvs = {'val':  epicsPV.epicsPV(name+'.VAL',  wait=False),
                    'dval': epicsPV.epicsPV(name+'.DVAL', wait=False),
                    'rval': epicsPV.epicsPV(name+'.RVAL', wait=False),
                    'rlv':  epicsPV.epicsPV(name+'.RLV',  wait=False),
                    'rbv':  epicsPV.epicsPV(name+'.RBV',  wait=False),
                    'drbv': epicsPV.epicsPV(name+'.DRBV', wait=False),
                    'rrbv': epicsPV.epicsPV(name+'.RRBV', wait=False),
                    'dmov': epicsPV.epicsPV(name+'.DMOV', wait=False),
                    'stop': epicsPV.epicsPV(name+'.STOP', wait=False),
                    'velo': epicsPV.epicsPV(name+'.VELO', wait=False),
                    'vbas': epicsPV.epicsPV(name+'.VBAS', wait=False),
                    'accl': epicsPV.epicsPV(name+'.ACCL', wait=False),
                    'desc': epicsPV.epicsPV(name+'.DESC', wait=False),
                    'mres': epicsPV.epicsPV(name+'.MRES', wait=False),
                    'hlm':  epicsPV.epicsPV(name+'.HLM',  wait=False),
                    'llm':  epicsPV.epicsPV(name+'.LLM',  wait=False),
                    'dhlm': epicsPV.epicsPV(name+'.DHLM', wait=False),
                    'dllm': epicsPV.epicsPV(name+'.DLLM', wait=False),
                    'bdst': epicsPV.epicsPV(name+'.BDST', wait=False),
                    'set':  epicsPV.epicsPV(name+'.SET',  wait=False),
                    'lvio': epicsPV.epicsPV(name+'.LVIO', wait=False),
                    'lls':  epicsPV.epicsPV(name+'.LLS',  wait=False),
                    'hls':  epicsPV.epicsPV(name+'.HLS',  wait=False),
                    'off':  epicsPV.epicsPV(name+'.OFF',  wait=False)
                    }
        # Wait for all PVs to connect
        self.pvs['val'].pend_io()
        self.pvs['dmov'].setMonitor()

    def move(self, value, relative=False, dial=False, step=False, ignore_limits=False):
        """
        Moves a motor to an absolute position or relative to the current position
        in user, dial or step coordinates.

        :param float value:   The absolute position or relative amount of the move
        :param bool relative: If True, move relative to current position. The default is an absolute move.
        :param bool dial: If True, _value_ is in dial coordinates. The default is user coordinates.
        :param bool step: If True, _value_ is in steps. The default is user coordinates.
        :param bool ignore_limits: If True, suppress raising exceptions if the move results in a soft or
                                   hard limit violation.
        :raises epicsMotorException: If software limit or hard limit violation detected,
                                     unless ignore_limits=True is set.

        .. note:: The "step" and "dial" keywords are mutually exclusive.
           The "relative" keyword can be used in user, dial or step coordinates.

        >>> m=epicsMotor('13BMD:m38')
        >>> m.move(10)          # Move to position 10 in user coordinates
        >>> m.move(50, dial=True)  # Move to position 50 in dial coordinates
        >>> m.move(2, step=True, relative=True) # Move 2 steps
        """
        if dial:
            # Position in dial coordinates
            if relative:
                current = self.get_position(dial=True)
                self.pvs['dval'].putw(current+value)
            else:
                self.pvs['dval'].putw(value)

        elif step:
            # Position in steps
            if relative:
                current = self.get_position(step=True)
                self.pvs['rval'].putw(current + value)
            else:
                self.pvs['rval'].putw(value)
        else:
            # Position in user coordinates
            if relative:
                self.pvs['rlv'].putw(value)
            else:
                self.pvs['val'].putw(value)

        # Check for limit violations
        if not ignore_limits:
            self.check_limits()

    def check_limits(self):
        """
        Check wether there is a soft limit, low hard limit or high hard limit violation.

        :raises epicsMotorException: If software limit or hard limit violation detected.
        """
        limit = self.pvs['lvio'].getw()
        if limit != 0:
            raise epicsMotorException('Soft limit violation')
        limit = self.pvs['lls'].getw()
        if limit != 0:
            raise epicsMotorException('Low hard limit violation')
        limit = self.pvs['hls'].getw()
        if limit != 0:
            raise epicsMotorException('High hard limit violation')

    def stop(self):
        """
        Immediately stops a motor from moving by writing 1 to the .STOP field.

        >>> m=epicsMotor('13BMD:m38')
        >>> m.move(10)          # Move to position 10 in user coordinates
        >>> m.stop()            # Stop motor
        """
        self.pvs['stop'].putw(1)

    def get_position(self, dial=False, readback=False, step=False):
        """
        Returns the target or readback motor position in user, dial or step
        coordinates.

        :param bool readback: If True, return the readback position in the
                              desired coordinate system. The default is to return the
                              target position of the motor.
        :param bool dial:     If True, return the position in dial coordinates.
                              The default is user coordinates.
        :param bool step:     If True, return the position in steps.
                              The default is user coordinates.


        .. note:: The "step" and "dial" keywords are mutually exclusive.
           The "readback" keyword can be used in user, dial or step coordinates.

        >>> m = epicsMotor('13BMD:m38')
        >>> m.move(10)                   # Move to position 10 in user coordinates
        >>> pos_dial = m.get_position(dial=True)   # Read the target position in dial coordinates
        >>> pos_step = m.get_position(readback=True, step=True) # Read the actual position in steps
        """
        if dial:
            if readback:
                return self.pvs['drbv'].getw()
            else:
                return self.pvs['dval'].getw()
        elif step:
            if readback:
                return self.pvs['rrbv'].getw()
            else:
                return self.pvs['rval'].getw()
        else:
            if readback:
                return self.pvs['rbv'].getw()
            else:
                return self.pvs['val'].getw()

    def set_position(self, position, dial=False, step=False):
        """
        Sets the motor position in user, dial or step coordinates.

        :param float position: The new motor position
        :param bool dial:      If True, set the position in dial coordinates.
                               The default is user coordinates.
        :param bool step:      If True, set the position in steps.
                               The default is user coordinates.

        .. note:: The "step" and "dial" keywords are mutually exclusive.

        >>> m = epicsMotor('13BMD:m38')
        >>> m.set_position(10, dial=True)   # Set the motor position to 10 in dial coordinates
        >>> m.set_position(1000, step=True) # Set the motor position to 1000 steps
        """
        # Put the motor in "SET" mode
        self.pvs['set'].putw(1)
        if dial:
            self.pvs['dval'].putw(position)
        elif step:
            self.pvs['rval'].putw(position)
        else:
            self.pvs['val'].putw(position)
        # Put the motor back in "Use" mode
        self.pvs['set'].putw(0)

    def wait(self, start=False, stop=False, poll=0.01, ignore_limits=False):
        """
        Waits for the motor to start moving and/or stop moving.

        :param bool start: If True, wait for the motor to start moving.
        :param bool stop:  If True, wait for the motor to stop moving.
        :param float poll: The time to wait between reading the
                           .DMOV field of the record to see if the motor is moving.
                           The default is 0.01 seconds.
        :param bool ignore_limits: If True, suppress raising an exception if a soft or
                                   hard limit is detected.
        :raises epicsMotorException: If software limit or hard limit violation detected,
                                     unless *ignore_limits=True* is set.


        .. note:: If neither the "start" nor "stop" keywords are set then "stop"
           is set to 1, so the routine waits for the motor to stop moving.
           If only "start" is set to 1 then the routine only waits for the
           motor to start moving.
           If both "start" and "stop" are set to 1 then the routine first
           waits for the motor to start moving, and then to stop moving.

        >>> m = epicsMotor('13BMD:m38')
        >>> m.move(50)                # Move to position 50
        >>> m.wait(start=True, stop=True)   # Wait for the motor to start moving and then to stop moving
        """
        if not start and not stop:
            stop = True
        if start:
            while self.pvs['dmov'].getw() == 1:
                time.sleep(poll)
        if stop:
            while self.pvs['dmov'].getw() == 0:
                time.sleep(poll)
        if not ignore_limits:
            self.check_limits()


class epicsMotorException(Exception):
    def __init__(self, message=''):
        self.message = message

    def __str__(self):
        return self.message

if __name__ == '__main__':
    import doctest
    doctest.testmod()
