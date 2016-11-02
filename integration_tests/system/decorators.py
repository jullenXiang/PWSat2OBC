from functools import wraps

from nose.tools import nottest
from build_config import config


def auto_comm_handling(enable_auto_comm):
    def wrap(f):
        @wraps(f)
        def wrapper(self, *args, **kwargs):
            self.system.obc.wait_to_start()
            self.system.obc.comm_auto_handling(enable_auto_comm)
            return f(self, *args, **kwargs)

        return wrapper

    return wrap


def wait_for_obc_start():
    def wrap(f):
        @wraps(f)
        def wrapper(self, *args, **kwargs):
            self.system.obc.wait_to_start()
            return f(self, *args, **kwargs)
        return wrapper
    return wrap


def require_two_i2c_buses(f):
    if config['SINGLE_BUS']:
        return nottest(f)
    else:
        return f
