#!/usr/bin/env python
import os
import unittest

from pycotap import TAPTestRunner

if __name__ == '__main__':
    tests_dir = os.path.join(
      os.path.dirname(os.path.abspath(__file__)), '../test'
      )
    loader = unittest.TestLoader()
    tests = loader.discover(tests_dir, pattern='*.py')
    runner = TAPTestRunner()
    runner.run(tests)
