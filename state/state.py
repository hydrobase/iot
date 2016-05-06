from datetime import datetime
import json
from configparser import ConfigParser
import operator
from pubnub import Pubnub
from pymongo import MongoClient


# configuration data
cfg = ConfigParser()
cfg.read('creds.cfg')
mongo_uri = cfg.get('mongo', 'MONGO_URI')
db_name = cfg.get('mongo', 'DB_NAME')
publish = cfg.get('pubnub', 'PUBNUB_PUBLISH_KEY')
subscribe = cfg.get('pubnub', 'PUBNUB_SUBSCRIBE_KEY')
secret = cfg.get('pubnub', 'PUBNUB_SECRET_KEY')
auth = cfg.get('pubnub', 'PUBNUB_AUTH_KEY')
channel_grp = cfg.get('pubnub', 'PUBNUB_CHANNEL_GRP')
# database instance
client = MongoClient(mongo_uri)
db = client[db_name]
# pubnub instance
pubnub = Pubnub(publish_key=publish, subscribe_key=subscribe,
                secret_key=secret, auth_key=auth)
pubnub.grant(channel_group=channel_grp, auth_key=auth, write=True)
# operators
compare = {'<' : operator.lt, '>' : operator.gt}

def connect_grows():
    """Get all of the grows from the database

    Parameters
    ----------
    db : pymongo.database.Database
        The database instance; assuming the URI is in `creds.cfg`

    Returns
    -------
    grows : pymongo.cursor.Cursor
        Iterable
    """
    grows = db.grows.find()
    return grows

def connect_data(d_id, g_name):
    """Get the most recent data for a device and grow

    Parameters
    ----------
    d_id : str
        Device id
    g_name : str
        Grow name

    Returns
    -------
    d : dict
        sensor data
    """
    data = db.data.find({'device_id' : d_id,
                         'grow_name' : g_name}).sort([['year', -1],
                                                      ['month', -1],
                                                      ['day', -1],
                                                      ['hour', -1],
                                                      ['min', -1],
                                                      ['sec', -1]]).limit(1)
    for d in data:
        return d

def device_id(obj):
    """Get the device ID

    Parameters
    ----------
    obj : dict
        A grow or data "object"

    Returns
    -------
    id : str
        Device id
    """
    return obj['device_id']

def grow_name(obj):
    """Get the grow name

    Parameters
    ----------
    obj : A grow or data "object"

    Returns
    -------
    id : str
        Grow name
    """
    return obj['grow_name']

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

def controls_dates(control):
    """Get the control start and end dates

    Parameters
    ----------
    control : dict
        A control "object"

    Returns
    -------
    start, end : datetime.date
        Will return None if no dates set
    """
    def date(str_date):
        return datetime.strptime(str_date, '%m/%d/%Y').date()
    try:
        start = control['dates']['start']
        end = control['dates']['end']
        return date(start), date(end)
    except:
        return None, None

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

def time_based_on(unit, value, action):
    """Determine whether a time-based
    control should be on or off

    Parameters
    ----------
    unit : str
        {'hours', 'minutes'}
    value : int
        User-defined time-interval value
    action : str
        {'toggle', 'on'}

    Returns
    -------
    bool
        True if device should be on
    """
    current = current_time(unit)
    if action == 'toggle':
        quotient = divmod(current, value)[0]
        return is_odd(quotient)
    elif action == 'on':
        return current < value

def condition_based_on(current, operator, value):
    """Determine whether a condition-based
    control should be on or off

    Parameters
    ----------
    current : int, float
        Most recent data for a specific sensor
    operator : str
        {'<', '>'}
    value : int, float
        The condition value

    Returns
    -------
    bool
        True if a device should be on
    """
    comp = compare[operator]
    return comp(current, value)

def n_values(obj, keys):
    """Extract multiple values from `obj`

    Parameters
    ----------
    obj : dict
        A grow or data "object"

    keys : list
        A list of valid key names
        Must have at least one element

    Returns
    -------
    tuple
        The values for each key in `args`

    Notes
    -----
    Always use tuple unpacking notation (commas); see examples

    Examples
    --------
    >>> x = {'one' : 1, 'two' : 2}
    >>> one, = n_values(x, ['one']) # include the comma even for a single key
    >>> one, two = n_values(x, ['one', 'two'])
    """
    assert isinstance(obj, dict), '`obj` must be type dict'
    assert isinstance(keys, list), '`keys` must be type list'
    return tuple(obj[k] for k in keys)

def pin_and_value(g, c, c_type='time', d=None):
    """Get the pin and its corresponding value

    Parameters
    ----------
    g : dict
        A grow "object"
    c : dict
        A control "object"
    c_type : str
        {'time', 'condition'}
    d : dict
        A data "object"
        Only used if `c_type` == 'condition'
    """
    actuator, = n_values(c, ['actuator'])
    pin = actuator_pin(g, actuator)
    if c_type == 'time':
        keys = ['unit', 'value', 'action']
        unit, value, action = n_values(c, keys)
        value = time_based_on(unit, value, action)
    elif c_type == 'condition':
        try:
            keys = ['sensor', 'operator', 'value', 'action']
            sensor, operator, value, action = n_values(c, keys)
            current = float(d[sensor])
            value = condition_based_on(current, operator, value)
            if action == 'on':
                value = value * True
            elif action == 'off':
                value = not value
        except TypeError as e:
            print(e)
    return pin, value * 255

def payload(g, c_type='time'):
    """Create the payload to publish to
    PubNub for a specific control type

    Parameters
    ----------
    g : dict
        A grow "object"
    c_type : str
        {'time', 'condition'}

    Returns
    -------
    p : dict
        A dict with pin : value pairs for a particular device ID

    Notes
    -----
    For time-based conditions (i.e., light or water pump),
    on is represented by a value of 255 and off by a value of 0
    """
    d_id = device_id(g)
    if c_type == 'time':
        controls = controls_time(g)
        d = None
    elif c_type == 'condition':
        controls = controls_condition(g)
        g_name = grow_name(g)
        d = connect_data(d_id, g_name)
    inner = {}
    for c in controls:
        start, end = controls_dates(c)
        # assuming no date restrictions if start and end dates not present
        if ((start is None) or (start <= datetime.now().date() <= end)):
            pin, v = pin_and_value(g, c, c_type, d)
            inner[pin] = str(v)
    if inner:
        p = {d_id : inner}
        return p

def control_messages(): # pragma: no cover
    """Publish to PubNub"""
    grows = connect_grows()
    for grow in grows:
        d_id = device_id(grow)
        message = payload(grow, 'time')
        m = payload(grow, 'condition')
        message[d_id].update(m[d_id])
        if message:
            pubnub.publish('admin', message)


if __name__ == '__main__': # pragma: no cover
    from time import sleep
    while True:
        sleep(8)
        control_messages()
