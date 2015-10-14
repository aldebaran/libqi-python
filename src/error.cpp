#include <qipython/error.hpp>

//http://stackoverflow.com/questions/1418015/how-to-get-python-exception-text
std::string PyFormatError()
{
  try
  {
    PyObject *exc,*val,*tb;
    boost::python::object formatted_list, formatted;
    PyErr_Fetch(&exc,&val,&tb);
    if (!exc)
    {
      static const char err[] = "Bug: no error after call to PyFormatError";
      qiLogError("python") << err;
      return err;
    }
    boost::python::handle<> hexc(exc),hval(boost::python::allow_null(val)),htb(boost::python::allow_null(tb));
    boost::python::object traceback(boost::python::import("traceback"));
    boost::python::object format_exception(traceback.attr("format_exception"));

    formatted_list = format_exception(hexc,hval,htb);
    formatted = boost::python::str("").join(formatted_list);
    return boost::python::extract<std::string>(formatted);
  }
  catch (...)
  {
    static const char err[] = "Bug: error while getting python error";
    qiLogError("python") << err;
    return err;
  }
}
