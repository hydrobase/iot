import unittest

from state.state import *


class TestCommunicate(unittest.TestCase):

    g = connect_grows()

    def test_grows_returns_dict(self):
        self.assertIsNotNone(self.g)
        self.assertIsInstance(self.g, dict)

    def test_device_id_str(self):
        g_id = device_id(self.g)
        self.assertIsNotNone(g_id)
        self.assertIsInstance(g_id, str)

    def test_controls_time_list(self):
        time = controls_time(self.g)
        self.assertIsNotNone(time)
        self.assertIsInstance(time, list)

    def test_controls_condition_list(self):
        condition = controls_condition(self.g)
        self.assertIsNotNone(condition)
        self.assertIsInstance(condition, list)

    def test_current_time(self):
        h = current_time('hours')
        self.assertIsInstance(h, int)
        self.assertTrue(0 <= h <= 23)
        m = current_time('minutes')
        self.assertIsInstance(m, int)
        self.assertTrue(0 <= m <= 59)

    def test_is_even(self):
        self.assertEquals(False, is_odd(2))
        self.assertEquals(True, is_odd(3))

    def test_time_hour_on(self):
        hour = 4
        self.assertEquals(False, time_based_on(0, hour))
        self.assertEquals(True, time_based_on(4, hour))
        self.assertEquals(True, time_based_on(5, hour))
        self.assertEquals(False, time_based_on(9, hour))
