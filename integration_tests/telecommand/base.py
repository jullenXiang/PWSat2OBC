from devices import UplinkFrame


class Telecommand:
    def __init__(self):
        pass

    def apid(self):
        raise NotImplementedError()

    def payload(self):
        raise NotImplementedError()

    def frame(self):
        return UplinkFrame(apid=self.apid(), content=self.payload())

    def build(self):
        return self.frame().build()