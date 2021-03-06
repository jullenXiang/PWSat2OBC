from datetime import datetime

from parameterized import parameterized

from system import runlevel
from tests.base import BaseTest, RestartPerTest
import logging
from utils import ensure_byte_list


def safe(l):
    return str(map(lambda x: ensure_byte_list(x), l))


class FileSystemTests(RestartPerTest):
    @runlevel(2)
    def test_write_read_file(self):
        log = logging.getLogger("test")

        log.info("Before write file")
        files = self.system.obc.list_files("/")
        log.info("Files: %s", safe(files))

        text = datetime.now().isoformat()
        self.system.obc.write_file("/test_file", text)

        log.info("After write file, before list files")
        files = self.system.obc.list_files("/")
        self.assertIn("test_file", files)

        log.info("Before power off")
        self.system.restart()

        log.info("After power off")
        files = self.system.obc.list_files("/")
        log.info("Files: %s", safe(files))

        read_back = self.system.obc.read_file("/test_file")

        log.info("After read")
        files = self.system.obc.list_files("/")
        log.info("Files: %s", safe(files))

        self.assertEqual(read_back, text)

    @runlevel(2)
    def test_write_read_long_file(self):
        path = "/file"
        data = '\n'.join(map(lambda x: x * 25, ['A', 'B', '>', 'C', 'D', 'E', 'F', 'G']))

        self.system.obc.write_file(path, data)

        files = self.system.obc.list_files("/")

        self.assertIn('file', files)

        read_back = self.system.obc.read_file(path)

        self.assertEqual(read_back, data)
