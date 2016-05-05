from datetime import datetime
import json
from configparser import ConfigParser
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

def connect_data():
    with open('data.json', 'r') as d:
        data = json.load(d)
    return data

def device_id(obj, obj_type='grow'):
    """Get the device ID

    Parameters
    ----------
    obj : dict
        A grow or data "object"
    obj_type : str
        {'grow', 'data'}

    Returns
    -------
    id : str
        Device id
    """
    assert obj_type in ('grow', 'data'), 'Invalid object type'
    if obj_type == 'grow':
        return obj['device_id']
    elif obj_type == 'data':
        return obj['sender']['device_id']

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
    """Determine whether a time-based control
    should be on or off

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
    d_id = device_id(g, 'grow')
    inner = {}
    for c in controls:
        start, end = controls_dates(c)
        # assuming no date restrictions if start and end dates not present
        if ((start is None) or (start <= datetime.now().date() <= end)):
            unit, value = c['unit'], c['value']
            actuator, action = c['actuator'], c['action']
            pin = actuator_pin(g, actuator)
            on_off_value = time_based_on(unit, value, action) * 255
            inner[pin] = str(on_off_value)
    if inner:
        p = {d_id : inner}
        return p

def control_messages(): # pragma: no cover
    grows = connect_grows()
    for grow in grows:
        message = payload(grow)
        if message:
            pubnub.publish('admin', message)
