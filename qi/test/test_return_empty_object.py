#!/usr/bin/env python

import qi
import pytest
import logging

logging.basicConfig(level=logging.INFO)

def test_return_empty_object(session_to_process_service_endpoint):
  service_name, service_session = session_to_process_service_endpoint
  log = logging.getLogger("test_return_empty_object")
  obj = service_session.service(service_name)
  prom = qi.Promise()
  def callback(obj):
    try:
      log.info("object is valid: {}.".format(obj.isValid()))
      prom.setValue(not obj.isValid())
    except Exception as err:
      prom.setError(str(err))
    except:
      prom.setError("Unknown exception.")
  obj.signal.connect(callback)
  obj.resetObject()
  obj.emit()
  assert prom.future().value()
