/*
**  Copyright (C) 2020 SoftBank Robotics Europe
**  See COPYING for the license
*/

#include <pybind11/pybind11.h>

#include <thread>
#include <chrono>
#include <future>

#include <qi/applicationsession.hpp>
#include <qi/atomic.hpp>
#include <qi/os.hpp>
#include <qi/log.hpp>

#include <qipython/common.hpp>
#include <qipython/pyguard.hpp>
#include <qipython/pyapplication.hpp>
#include <qipython/pysession.hpp>

qiLogCategory("qi.python.application");

namespace py = pybind11;

namespace qi
{
namespace py
{

namespace
{

template<typename F, typename... ArgsExtras>
struct WithArgcArgv
{
  F f;

  auto operator()(::py::list args, ArgsExtras... extras) const
    -> decltype(f(std::declval<int&>(),
                  std::declval<char**&>(),
                  std::forward<ArgsExtras>(extras)...))
  {
    std::vector<std::string> argsStr;
    std::transform(args.begin(), args.end(), std::back_inserter(argsStr),
                   [](const ::py::handle& arg){ return arg.cast<std::string>(); });

    std::vector<char*> argsCStr;
    std::transform(argsStr.begin(), argsStr.end(), std::back_inserter(argsCStr),
                   [](const std::string& s) { return const_cast<char*>(s.c_str()); });

    int argc = argsCStr.size();
    char** argv = argsCStr.data();

    auto res = f(argc, argv, std::forward<ArgsExtras>(extras)...);

    // The constructor of `qi::Application` modifies its parameters to
    // consume the arguments it processed. After the constructor, the first
    // `argc` elements of `argv` are the arguments that were not recognized by
    // the application. We then have to propagate the consumption of the
    // elements to Python, by clearing the list then reinserting the elements
    // that were left.
    args.attr("clear")();
    for (std::size_t i = 0; i < static_cast<std::size_t>(argc); ++i)
      args.insert(i, argv[i]);

    return res;
  }
};

template<typename... ExtraArgs, typename F>
WithArgcArgv<ka::Decay<F>, ExtraArgs...> withArgcArgv(F&& f)
{
  return { std::forward<F>(f) };
}

} // namespace

void exportApplication(::py::module& m)
{
  using namespace ::py;
  using namespace ::py::literals;

  GILAcquire lock;

  class_<Application, std::unique_ptr<Application, DeleteInOtherThread>>(
    m, "Application")
    .def(init(withArgcArgv<>([](int& argc, char**& argv) {
           GILRelease unlock;
           return new Application(argc, argv);
         })),
         "args"_a)
    .def_static("run", &Application::run, call_guard<GILRelease>())
    .def_static("stop", &Application::stop, call_guard<GILRelease>());

  class_<ApplicationSession,
         std::unique_ptr<ApplicationSession, DeleteInOtherThread>>(
    m, "ApplicationSession")

    .def(init(withArgcArgv<bool, const std::string&>(
           [](int& argc, char**& argv, bool autoExit, const std::string& url) {
             GILRelease unlock;
             ApplicationSession::Config config;
             if (!autoExit)
               config.setOption(qi::ApplicationSession::Option_NoAutoExit);
             if (!url.empty())
               config.setConnectUrl(url);
             return new ApplicationSession(argc, argv, config);
           })),
         "args"_a, "autoExit"_a, "url"_a)

    .def("run", &ApplicationSession::run, call_guard<GILRelease>(),
         doc("Block until the end of the program (call "
             ":py:func:`qi.ApplicationSession.stop` to end the program)."))

    .def_static("stop", &ApplicationSession::stop,
                call_guard<GILRelease>(),
                doc(
                  "Ask the application to stop, the run function will return."))

    .def("start", &ApplicationSession::startSession,
         call_guard<GILRelease>(),
         doc("Start the connection of the session, once this function is "
             "called everything is fully initialized and working."))

    .def_static("atRun", &ApplicationSession::atRun,
                call_guard<GILRelease>(), "func"_a,
                doc(
                  "Add a callback that will be executed when run() is called."))

    .def_property_readonly("url",
                           [](const ApplicationSession& app) {
                             return app.url().str();
                           },
                           call_guard<GILRelease>(),
                           doc("The url given to the Application. It's the url "
                               "used to connect the session."))

    .def_property_readonly("session",
                           [](const ApplicationSession& app) {
                             return makeSession(app.session());
                           },
                           doc("The session associated to the application."));
}

} // namespace py
} // namespace qi
