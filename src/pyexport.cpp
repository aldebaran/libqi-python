/*
**  Copyright (C) 2020 SoftBank Robotics Europe
**  See COPYING for the license
*/

#include <qipython/pyexport.hpp>
#include <qipython/common.hpp>
#include <qipython/pyguard.hpp>
#include <qipython/pytypes.hpp>
#include <qipython/pyapplication.hpp>
#include <qipython/pyfuture.hpp>
#include <qipython/pysession.hpp>
#include <qipython/pyobject.hpp>
#include <qipython/pysignal.hpp>
#include <qipython/pyproperty.hpp>
#include <qipython/pymodule.hpp>
#include <qipython/pyasync.hpp>
#include <qipython/pylog.hpp>
#include <qipython/pypath.hpp>
#include <qipython/pytranslator.hpp>
#include <qipython/pyclock.hpp>
#include <qipython/pystrand.hpp>

namespace py = pybind11;

namespace qi
{
namespace py
{

void exportAll(pybind11::module& module)
{
  registerTypes();

  GILAcquire lock;

  exportFuture(module);
  exportSignal(module);
  exportProperty(module);
  exportObject(module);
  exportSession(module);
  exportApplication(module);
  exportObjectFactory(module);
  exportAsync(module);
  exportLog(module);
  exportPath(module);
  exportTranslator(module);
  exportStrand(module);
  exportClock(module);
}

} // namespace py
} // namespace qi
