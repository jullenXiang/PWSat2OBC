import telecommand
from system import auto_power_on
from tests.base import BaseTest
from utils import ensure_byte_list, TestEvent


class CommTelecommandsTest(BaseTest):
    @auto_power_on(auto_power_on=False)
    def __init__(self, *args, **kwargs):
        super(CommTelecommandsTest, self).__init__(*args, **kwargs)

    def _start(self):
        e = TestEvent()

        def on_reset(_):
            e.set()
        self.system.comm.on_hardware_reset = on_reset

        self.system.obc.power_on(clean_state=True)
        self.system.obc.wait_to_start()

        e.wait_for_change(1)

    def test_enter_idle_state(self):
        event = TestEvent()

        def on_set_idle_state(state):
            if state:
                event.set()

        self.system.transmitter.on_set_idle_state = on_set_idle_state

        self._start()

        self.system.comm.put_frame(telecommand.EnterIdleState(correlation_id=0x11, duration=5))

        frame = self.system.comm.get_frame(20)

        self.assertEqual(frame.apid(), 2)
        self.assertEqual(frame.seq(), 0)
        self.assertEqual(frame.payload(), [0x11, 0])

        self.assertTrue(event.wait_for_change(1))

    def test_leave_idle_state(self):
        event = TestEvent()

        def on_set_idle_state(state):
            if not state:
                event.set()

        self.system.transmitter.on_set_idle_state = on_set_idle_state

        self._start()

        self.system.comm.put_frame(telecommand.EnterIdleState(correlation_id=0x11, duration=1))

        frame = self.system.comm.get_frame(20)

        self.assertEqual(frame.apid(), 2)
        self.assertEqual(frame.seq(), 0)
        self.assertEqual(frame.payload(), [0x11, 0])

        self.assertTrue(event.wait_for_change(30))