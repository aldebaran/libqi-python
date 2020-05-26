include_guard(GLOBAL)

if(QIPYTHON_STANDALONE)
  include(set_install_rules_standalone)
else()
  include(set_install_rules_system)
endif()
