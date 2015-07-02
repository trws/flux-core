import re
import os
import sys
import json
import subprocess
import contextlib
import errno
import pprint
import shutil
import tempfile
import time

# pprint.pprint(os.environ)
flux_exe = ''
if os.environ.get('CHECK_BUILDDIR', None) is not None:
  flux_exe = os.path.abspath(os.environ['CHECK_BUILDDIR'] + '/src/cmd/flux')
else:
  flux_exe = os.path.abspath(os.path.dirname(os.path.abspath(__file__)) + '/../../../cmd/flux')

@contextlib.contextmanager
def get_tmpdir():
  d = tempfile.mkdtemp()
  try:
    yield d
  finally:
    shutil.rmtree(d)

@contextlib.contextmanager
def run_beside_flux(size=1):
  global flux_exe

  flux_command = [flux_exe, '--verbose', 'start', '--size={}'.format(size), '-o', '--verbose,-L,stderr', """bash -c 'echo READY ; while true ; do sleep 1; done' """]
  print ' '.join(flux_command)
  FNULL = open(os.devnull, 'w+')

  f = subprocess.Popen(flux_command,
      stdout=subprocess.PIPE,
      stderr=subprocess.STDOUT,
      close_fds=True,
      preexec_fn=os.setsid,# Start a process session to clean up brokers
      )
  while True:
    line = f.stdout.readline()
    print line
    if line != '':
      m = re.match(r"\s+(FLUX_[^=]+)=(.*)", line.rstrip())
      if m:
        print "setting", m.group(1), "to", os.path.abspath(m.group(2))
        os.environ[m.group(1)] = os.path.abspath(m.group(2))
      m = re.match(r'lt-flux-broker: FLUX_TMPDIR: (.*)', line.rstrip())
      if m:
        print "setting", "FLUX_TMPDIR", "to", os.path.abspath(m.group(1))
        os.environ['FLUX_TMPDIR'] = m.group(1)
      if re.search('READY', line):
        break
    else:
      break
  time.sleep(0.1)
  # print json.dumps(dict(os.environ))
  try:
    yield f
  finally:
    # Kill the process group headed by the subprocess
    os.killpg(f.pid, 15)


if __name__ == '__main__':
  with run_beside_flux(1):
    while True:
      pass
