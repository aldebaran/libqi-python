#ifndef QIPY_STRAND_HELPERS
#define QIPY_STRAND_HELPERS

#include <memory>
#include <boost/python/object.hpp>
#include <qi/strand.hpp>

namespace qi
{

namespace py
{

extern const char* const objectAttributeStrandName;
extern const char* const objectAttributeThreadingName;
extern const char* const objectAttributeThreadingValueMulti;
extern const char* const objectAttributeImSelfName;

// This deleter is necessary to make sure we delete the object
// outside of the lock of the GIL.
struct DeleterPyStrand {
  void operator()(qi::Strand* strand) const;
};

using PyStrand = std::shared_ptr<qi::Strand>;


/**
 * \return the strand if the object is a functor bound to an actor, nullptr otherwise
 */
qi::Strand* extractStrandFromCallable(const boost::python::object& callable);
/**
 * \return the strand if the python object has one, nullptr otherwise
 */
qi::Strand* extractStrandFromObject(const boost::python::object& obj);

/// Returns true if the object was declared as multithreaded, false otherwise.
bool isMultithreaded(const boost::python::object& obj);

void export_pystrand();

}
}

#endif
