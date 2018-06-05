#pragma once

/*
**  Copyright (C) 2014 Aldebaran Robotics
**  See COPYING for the license
*/

#ifndef PYQIOBJECT_P_HPP_
#define PYQIOBJECT_P_HPP_

namespace qi
{
namespace py
{

struct PyQiFunctor
{
public:
  PyQiFunctor(const std::string& funName, qi::AnyObject obj)
      : _object(obj), _funName(funName)
  {
  }

  ~PyQiFunctor();

  boost::python::object operator()(boost::python::tuple pyargs,
                                   boost::python::dict pykwargs);

public:
  qi::AnyObject _object;
  std::string _funName;
};

class PyQiObject
{
public:
  PyQiObject()
  {
  }

  PyQiObject(const qi::AnyObject& obj) : _object(obj)
  {
  }

  ~PyQiObject();

  boost::python::object call(boost::python::str pyname,
                             boost::python::tuple pyargs,
                             boost::python::dict pykws)
  {
    return PyQiFunctor(boost::python::extract<std::string>(pyname), _object)(
        pyargs, pykws);
  }

  bool operator==(const PyQiObject& x) const
  {
    return _object == x._object;
  }

  bool operator!=(const PyQiObject& x) const
  {
    return _object != x._object;
  }

  bool operator<(const PyQiObject& x) const
  {
    return _object < x._object;
  }

  bool operator<=(const PyQiObject& x) const
  {
    return _object <= x._object;
  }

  bool operator>(const PyQiObject& x) const
  {
    return _object > x._object;
  }

  bool operator>=(const PyQiObject& x) const
  {
    return _object >= x._object;
  }

  bool isValid() const
  {
    return _object.isValid();
  }

  boost::python::object metaObject()
  {
    return qi::AnyReference::from(_object.metaObject())
        .to<boost::python::object>();
  }

  qi::AnyObject object()
  {
    return _object;
  }

private:
  qi::AnyObject _object;
};

template <typename T>
boost::python::object pyParamShrinker(boost::python::tuple args,
                                      boost::python::dict kwargs)
{
  T& pys = boost::python::extract<T&>(args[0]);
  boost::python::list l;
  for (int i = 2; i < boost::python::len(args); ++i) l.append(args[i]);
  return pys.call(boost::python::extract<boost::python::str>(args[1]),
                  boost::python::tuple(l), kwargs);
}

template <typename T>
boost::python::object pyParamShrinkerAsync(boost::python::tuple args,
                                           boost::python::dict kwargs)
{
  T& pys = boost::python::extract<T&>(args[0]);
  boost::python::list l;
  for (int i = 2; i < boost::python::len(args); ++i) l.append(args[i]);
  kwargs["_async"] = true;
  return pys.call(boost::python::extract<boost::python::str>(args[1]),
                  boost::python::tuple(l), kwargs);
}
}
}

#endif // PYQIOBJECT_P_HPP_
