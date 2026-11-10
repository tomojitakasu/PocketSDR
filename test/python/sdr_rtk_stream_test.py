#!/usr/bin/env python3
#
#  unit test driver for Python RTKLIB stream wrappers
#
import subprocess, sys, tempfile, unittest
from pathlib import Path
import numpy as np

PYTHON_DIR = Path(__file__).resolve().parents[2] / 'python'
sys.path.insert(0, str(PYTHON_DIR))
import sdr_rtk


class StreamTest(unittest.TestCase):
    # verify byte counters and file data for valid read and write streams --------
    def test_stream_summary(self):
        data = np.array([0, 1, 127, 128, 255], dtype='uint8')
        with tempfile.TemporaryDirectory() as directory:
            path = str(Path(directory) / 'stream.bin')
            stream = sdr_rtk.stropen(sdr_rtk.STR_FILE, sdr_rtk.STR_MODE_W, path)
            self.assertIsNotNone(stream)
            try:
                self.assertEqual(sdr_rtk.strsum(stream), (0, 0, 0, 0))
                self.assertEqual(sdr_rtk.strwrite(stream, data), len(data))
                inb, inr, outb, outr = sdr_rtk.strsum(stream)
                self.assertEqual((inb, outb), (0, len(data)))
                self.assertGreaterEqual(inr, 0)
                self.assertGreaterEqual(outr, 0)
            finally:
                sdr_rtk.strclose(stream)
            self.assertEqual(Path(path).read_bytes(), data.tobytes())
            stream = sdr_rtk.stropen(sdr_rtk.STR_FILE, sdr_rtk.STR_MODE_R, path)
            self.assertIsNotNone(stream)
            try:
                received = np.zeros(len(data), dtype='uint8')
                self.assertEqual(sdr_rtk.strread(stream, received), len(data))
                np.testing.assert_array_equal(received, data)
                inb, inr, outb, outr = sdr_rtk.strsum(stream)
                self.assertEqual((inb, outb), (len(data), 0))
                self.assertGreaterEqual(inr, 0)
                self.assertGreaterEqual(outr, 0)
            finally:
                sdr_rtk.strclose(stream)

    # isolate null-pointer regressions from the test runner ---------------------
    def check_null_call(self, call):
        code = ('import sys; sys.path.insert(0, %r); '
                'import numpy as np; import sdr_rtk; ' % str(PYTHON_DIR)) + call
        result = subprocess.run([sys.executable, '-c', code],
                                capture_output=True, text=True, timeout=10)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    # closing a failed stream is a no-op ----------------------------------------
    def test_close_none(self):
        self.check_null_call('assert sdr_rtk.strclose(None) is None')

    # writing a failed stream reports zero bytes --------------------------------
    def test_write_none(self):
        self.check_null_call("assert sdr_rtk.strwrite(None, np.array([1], dtype='uint8')) == 0")

    # preserve rejection of unsupported buffer types ----------------------------
    def test_write_invalid_dtype(self):
        with tempfile.TemporaryDirectory() as directory:
            path = str(Path(directory) / 'stream.bin')
            stream = sdr_rtk.stropen(sdr_rtk.STR_FILE, sdr_rtk.STR_MODE_W, path)
            self.assertIsNotNone(stream)
            try:
                self.assertEqual(sdr_rtk.strwrite(stream, np.array([1], dtype='int8')), 0)
            finally:
                sdr_rtk.strclose(stream)
            self.assertEqual(Path(path).read_bytes(), b'')


if __name__ == '__main__':
    unittest.main()
