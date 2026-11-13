#!/usr/bin/env python3
#
#  unit test driver for Python log streams
#
import os, socket, sys, tempfile, time, unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'python'))
import sdr_func


class LogStreamTest(unittest.TestCase):
    # restore log settings after each test --------------------------------------
    def tearDown(self):
        if sdr_func.log_str:
            sdr_func.log_close()
        sdr_func.log_level(3)

    # write a log message to a file --------------------------------------------
    def check_file(self, path):
        sdr_func.log_open(path)
        self.assertIsNotNone(sdr_func.log_str)
        sdr_func.log(3, '$LOG,test')
        sdr_func.log_close()
        with open(path, 'rb') as file:
            self.assertEqual(file.read(), b'$LOG,test\r\n')

    # preserve relative file paths ---------------------------------------------
    def test_file(self):
        with tempfile.TemporaryDirectory(dir='.') as directory:
            self.check_file(os.path.relpath(os.path.join(directory, 'receiver.log')))

    # preserve absolute paths including Windows drive letters ------------------
    def test_absolute_file(self):
        with tempfile.TemporaryDirectory() as directory:
            self.check_file(os.path.abspath(os.path.join(directory, 'receiver.log')))

    # send a log message through the TCP client --------------------------------
    def test_tcp_client(self):
        with socket.socket() as server:
            server.bind(('127.0.0.1', 0))
            server.listen(1)
            server.settimeout(0.05)
            sdr_func.log_open('127.0.0.1:%d' % server.getsockname()[1])
            self.assertIsNotNone(sdr_func.log_str)
            deadline = time.monotonic() + 3.0
            connection = None
            while time.monotonic() < deadline:
                sdr_func.log(3, '$LOG,test')
                try:
                    connection, _ = server.accept()
                    break
                except socket.timeout:
                    pass
            self.assertIsNotNone(connection, 'TCP client did not connect')
            with connection:
                connection.settimeout(0.05)
                received = b''
                while time.monotonic() < deadline and b'$LOG,test\r\n' not in received:
                    sdr_func.log(3, '$LOG,test')
                    try:
                        received += connection.recv(1024)
                    except socket.timeout:
                        pass
                self.assertIn(b'$LOG,test\r\n', received)

    # send a log message through the TCP server --------------------------------
    def test_tcp_server(self):
        with socket.socket() as probe:
            probe.bind(('127.0.0.1', 0))
            port = probe.getsockname()[1]
        sdr_func.log_open(':%d' % port)
        self.assertIsNotNone(sdr_func.log_str)
        with socket.create_connection(('127.0.0.1', port), timeout=3.0) as client:
            client.settimeout(0.05)
            deadline = time.monotonic() + 3.0
            received = b''
            while time.monotonic() < deadline and b'$LOG,test\r\n' not in received:
                sdr_func.log(3, '$LOG,test')
                try:
                    received += client.recv(1024)
                except socket.timeout:
                    pass
            self.assertIn(b'$LOG,test\r\n', received)


if __name__ == '__main__':
    unittest.main()
