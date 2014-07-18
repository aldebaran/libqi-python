#pragma once
/*
**  Copyright (C) 2013 Aldebaran Robotics
**  See COPYING for the license
*/

#ifndef _QIPYTHON_PYOBJECTFACTORY_HPP_
#define _QIPYTHON_PYOBJECTFACTORY_HPP_

#include <qipython/api.hpp>
#include <qi/anymodule.hpp>
#include <string>

namespace qi {
  namespace py {
    void export_pyobjectfactory();

    QIPYTHON_API qi::AnyModule importPyModule(const qi::ModuleInfo& name);
  }
}

#endif  // _QIPYTHON_PYOBJECTFACTORY_HPP_
