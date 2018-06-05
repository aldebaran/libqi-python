import pytest
import qi
import subprocess
import itertools

@pytest.fixture
def session(request, url):
  """ Connected session to a URL. """
  sess = qi.Session()
  sess.connect(url)
  request.addfinalizer(sess.close)
  return sess

@pytest.fixture
def process(request, exec_path):
  """ Process launched from an executable path. """
  proc = subprocess.Popen(exec_path.split(" "), stdout=subprocess.PIPE, universal_newlines=True)
  def fin():
    proc.terminate()
    proc.wait()
  request.addfinalizer(fin)
  return proc

@pytest.fixture
def process_service_endpoint(process):
  """ Endpoint of a service with its name that a process returns on its standard output. """
  service_name_prefix = "service_name="
  endpoint_prefix = "endpoint="
  service_name, service_endpoint = str(), str()
  # iterating over pipes doesn't work in python 2.7
  for line in itertools.imap(str.strip, iter(process.stdout.readline, "")):
    if line.startswith(endpoint_prefix):
      service_endpoint = line[len(endpoint_prefix):]
    elif line.startswith(service_name_prefix):
      service_name = line[len(service_name_prefix):]
    if len(service_endpoint) > 0 and len(service_name) > 0:
      break
  return service_name, service_endpoint

@pytest.fixture
def session_to_process_service_endpoint(request, process_service_endpoint):
  service_name, service_endpoint = process_service_endpoint
  return service_name, session(request, service_endpoint)

def pytest_addoption(parser):
  parser.addoption('--url', action='append', type=str, default=[],
                   help='Url to connect the session to.')
  parser.addoption('--exec_path', action='append', type=str, default=[],
                   help='Executable with arguments to launch before the test.')

def pytest_generate_tests(metafunc):
  if 'url' in metafunc.fixturenames:
    metafunc.parametrize("url", metafunc.config.getoption('url'))
  if 'exec_path' in metafunc.fixturenames:
    metafunc.parametrize("exec_path", metafunc.config.getoption('exec_path'))
