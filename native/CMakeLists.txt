add_library(multileveldb SHARED src/multileveldb.cpp src/windows_env.cpp)
target_compile_features(multileveldb PRIVATE cxx_std_17)
target_compile_definitions(multileveldb PRIVATE ML_BUILD)
target_compile_definitions(multileveldb PRIVATE ML_HAVE_SNAPPY=$<BOOL:${HAVE_SNAPPY}>)
target_compile_definitions(multileveldb PRIVATE ML_LIBRARY_VERSION="${PROJECT_VERSION}")
target_include_directories(multileveldb PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(multileveldb PRIVATE leveldb)
set_target_properties(multileveldb PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION 1
  CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN YES)
if(WIN32)
  set_target_properties(multileveldb PROPERTIES PREFIX "")
endif()
if(MSVC)
  target_compile_options(multileveldb PRIVATE /W4 /EHsc /utf-8)
else()
  target_compile_options(multileveldb PRIVATE -Wall -Wextra -Wpedantic)
endif()
install(TARGETS multileveldb EXPORT multileveldbTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES include/multileveldb.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT multileveldbTargets NAMESPACE multileveldb:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/multileveldb)
include(CMakePackageConfigHelpers)
configure_package_config_file(multileveldbConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/multileveldbConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/multileveldb)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/multileveldbConfigVersion.cmake
  VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/multileveldbConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/multileveldbConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/multileveldb)
