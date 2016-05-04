from datetime import datetime
import json


def connect_grows():
    with open('grows.json', 'r') as g:
        grows = json.load(g)
    return grows

def device_id(g):
    """Get the device ID

    Parameters
    ----------
    g : dict
        A grow "object"

    Returns
    -------
    id : str
        Device id
    """
    return g['device_id']

def actuator_pin(g, actuator=None):
    """Get the actuator pin

    Parameters
    ----------
    g : dict
        A grow "object"
    actuator : str
        Actuator name

    Returns
    -------
    pin : str
        Actuator pin
    """
    pin = g['actuators'][actuator]
    return pin

def controls_time(g):
    """Get the time-based control information

    Parameters
    ----------
    g : dict
        A grow "object"

    Returns
    -------
    time : list
        Time-based controls
    """
    return g['controls']['time']

def controls_condition(g):
    """Get the condition-based control information

    Parameters
    ----------
    g : dict
        A grow "object"

    Returns
    -------
    condition : list
        Condition-based controls
    """
    return g['controls']['condition']

def current_time(unit='hours'):
    """Get the current time in hours or minutes

    Parameters
    ----------
    unit : str
        {'hours', 'minutes'}

    Returns
    -------
    value : int
        based on the specified unit
    """
    assert unit in ('hours', 'minutes'), 'Invalid unit'
    if unit == 'hours':
        value = datetime.now().time().hour
    elif unit == 'minutes':
        value = datetime.now().time().minute
    return value

def is_odd(v):
    """Determine whether a given integer is odd

    Parameters
    ----------
    v : int
        input value

    Returns
    -------
    bool
        True if odd, False otherwise
    """
    return v % 2 != 0

def time_based_on(unit, value):
    """Determine whether a time-based control
    should be on or off

    Parameters
    ----------
    unit : str
        {'hours', 'minutes'}
    value : int
        User-defined time-interval value

    Returns
    -------
    bool
        True if device should be on
    """
    current = current_time(unit)
    quotient = divmod(current, value)[0]
    return is_odd(quotient)

def payload(g):
    """Create the payload to publish to PubNub

    Parameters
    ----------
    g : dict
        A growth "object"

    Returns
    -------
    p : dict
        A dict with pin : value pairs for a particular device ID

    Notes
    -----
    For time-based conditions (i.e., light or water pump),
    on is represented by a value of 255 and off by a value of 0
    """
    controls = controls_time(g)
    d_id = device_id(g)
    p = {d_id : {}}
    for c in controls:
        actuator, unit, value = c['actuator'], c['unit'], c['value']
        pin = actuator_pin(g, actuator)
        on_off_value = time_based_on(unit, value) * 255
        p[d_id][pin] = str(on_off_value)
    return p
