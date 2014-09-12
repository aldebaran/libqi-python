#ifndef QIPY_STRAND_HELPERS
#define QIPY_STRAND_HELPERS

#include <boost/python/object.hpp>

namespace qi
{

class Strand;

namespace py
{

qi::Strand* extractStrand(const boost::python::object& obj);

void export_pystrand();

}
}

#endif
