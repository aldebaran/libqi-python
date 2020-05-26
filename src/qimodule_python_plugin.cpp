/*
**  Copyright (C) 2020 SoftBank Robotics Europe
**  See COPYING for the license
*/

#include <qi/anymodule.hpp>
#include <qi/log.hpp>

qiLogCategory("qi.python.module");

namespace
{

// Python AnyModule Factory
qi::AnyModule importPyModule(const qi::ModuleInfo& /*name*/)
{
  qiLogInfo() << "import in python not implemented yet";
  return qi::AnyModule();
}

} // namespace

QI_REGISTER_MODULE_FACTORY_PLUGIN("python", &::importPyModule);
