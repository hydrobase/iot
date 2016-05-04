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

def current_time(unit='hour'):
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

def time_based_on(current, value):
    """Determine whether a time-based control
    should be on or off

    Parameters
    ----------
    current : int
        Current value for a given unit (e.g., 'hours')
    value : int
        User-defined time-interval value

    Returns
    -------
    bool
        True if device should be on

    Notes
    -----
    `current` and `value` should be in the same units
    """
    quotient = divmod(current, value)[0]    
    return is_odd(quotient)
