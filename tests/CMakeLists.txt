if(QI_WITH_TESTS)
  find_package(qimodule)
  qi_sanitize_compile_flags(HIDDEN_SYMBOLS)
  qi_create_module(moduletest api.hpp moduletest.cpp SHARED DEPENDS QI NO_INSTALL)
  install(TARGETS moduletest DESTINATION lib COMPONENT test)

  qi_create_test_helper(
    service_object_holder
    SRC
      "service_object_holder.cpp"
    DEPENDS QI
  )
endif()
