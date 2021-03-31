#
# Copyright (C) 2010 - 2020 Softbank Robotics Europe
#
# -*- coding: utf-8 -*-

import qi
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
        except:  # noqa: E722
            prom.setError("Unknown exception.")
            raise
    obj.signal.connect(callback)
    obj.resetObject()
    obj.emit()
    assert prom.future().value()
