add_library(fkYAML INTERFACE)
target_include_directories(fkYAML INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(fkYAML PROPERTIES FOLDER "deps")
