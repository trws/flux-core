import unittest
import errno
import flux.core as core
import flux.json_c
import tap

json_str = '{"a":42}';
class TestRequestMethods(unittest.TestCase):

  def test_no_topic_invalid(self):
    """flux_request_encode returns EINVAL with no topic string"""
    with self.assertRaises(EnvironmentError) as err:
        core.raw.request_encode(None, json_str)
    err = err.exception
    self.assertEqual(err.errno, errno.EINVAL)

  def test_null_payload(self):
    """flux_request_encode works with NULL payload"""
    self.assertTrue(
        core.raw.request_encode("foo.bar", None) is not None
        )

if __name__ == '__main__':
      unittest.main()
