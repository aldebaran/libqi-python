/*
** Author(s):
**  - Cedric GESTES <cgestes@aldebaran.com>
**
** Copyright (C) 2014 Aldebaran
*/

#include <qipython/pymodule.hpp>


QI_REGISTER_MODULE_FACTORY_PLUGIN("python", &qi::py::importPyModule);

