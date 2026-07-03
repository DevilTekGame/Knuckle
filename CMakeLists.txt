cmake_minimum_required(VERSION 3.21)
project(Knuckle VERSION 1.0.0 LANGUAGES C CXX)

include("${CMAKE_CURRENT_SOURCE_DIR}/CMake/DownloadCEF.cmake")
list(APPEND CMAKE_MODULE_PATH "${CEF_ROOT}/cmake")
find_package(CEF REQUIRED)

add_subdirectory(${CEF_LIBCEF_DLL_WRAPPER_PATH} libcef_dll_wrapper)

set(KNUCKLE_SRCS
  src/main.cc
  src/handler.h
  src/handler.cc
)

if(OS_WINDOWS)
  list(APPEND KNUCKLE_SRCS src/resources/app.rc)
endif()

SET_CEF_TARGET_OUT_DIR()

if(OS_WINDOWS)
  add_executable(Knuckle WIN32 ${KNUCKLE_SRCS})
  SET_EXECUTABLE_TARGET_PROPERTIES(Knuckle)
  ADD_LOGICAL_TARGET("libcef_lib" "${CEF_LIB_DEBUG}" "${CEF_LIB_RELEASE}")
  add_dependencies(Knuckle libcef_dll_wrapper)
  target_link_libraries(Knuckle libcef_lib libcef_dll_wrapper ${CEF_STANDARD_LIBS})
  COPY_FILES("Knuckle" "${CEF_BINARY_FILES}" "${CEF_BINARY_DIR}" "${CEF_TARGET_OUT_DIR}")
  COPY_FILES("Knuckle" "${CEF_RESOURCE_FILES}" "${CEF_RESOURCE_DIR}" "${CEF_TARGET_OUT_DIR}")
elseif(OS_LINUX)
  add_executable(Knuckle ${KNUCKLE_SRCS})
  SET_EXECUTABLE_TARGET_PROPERTIES(Knuckle)
  ADD_LOGICAL_TARGET("libcef_lib" "${CEF_LIB_DEBUG}" "${CEF_LIB_RELEASE}")
  add_dependencies(Knuckle libcef_dll_wrapper)
  target_link_libraries(Knuckle libcef_lib libcef_dll_wrapper ${CEF_STANDARD_LIBS})
  set_target_properties(Knuckle PROPERTIES INSTALL_RPATH "$ORIGIN")
  set_target_properties(Knuckle PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)
  set_target_properties(Knuckle PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CEF_TARGET_OUT_DIR})
  COPY_FILES("Knuckle" "${CEF_BINARY_FILES}" "${CEF_BINARY_DIR}" "${CEF_TARGET_OUT_DIR}")
  COPY_FILES("Knuckle" "${CEF_RESOURCE_FILES}" "${CEF_RESOURCE_DIR}" "${CEF_TARGET_OUT_DIR}")
elseif(OS_MAC)
  add_executable(Knuckle MACOSX_BUNDLE ${KNUCKLE_SRCS})
  SET_EXECUTABLE_TARGET_PROPERTIES(Knuckle)
  add_dependencies(Knuckle libcef_dll_wrapper)
  target_link_libraries(Knuckle libcef_dll_wrapper ${CEF_STANDARD_LIBS})
  COPY_MAC_FRAMEWORK("Knuckle" "${CEF_BINARY_DIR}" "${CEF_TARGET_OUT_DIR}/Knuckle.app")
endif()

add_custom_command(TARGET Knuckle POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/web"
    "${CEF_TARGET_OUT_DIR}/web"
  COMMENT "Copying web resources")

# ── package target: create portable archive ──────────────────────────────
set(PACKAGE_NAME "Knuckle-${PROJECT_VERSION}")
set(PACKAGE_DIR "${CMAKE_BINARY_DIR}/${PACKAGE_NAME}")

add_custom_target(package
  DEPENDS Knuckle
  COMMAND ${CMAKE_COMMAND} -E make_directory "${PACKAGE_DIR}"
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${CEF_TARGET_OUT_DIR}" "${PACKAGE_DIR}"
  COMMAND ${CMAKE_COMMAND} -E remove -f "${PACKAGE_DIR}/web"
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/web" "${PACKAGE_DIR}/web"
  COMMENT "Packaging ${PACKAGE_NAME}"
  VERBATIM)

if(OS_WINDOWS)
  set(PACKAGE_ARCHIVE "${CMAKE_BINARY_DIR}/${PACKAGE_NAME}.zip")
  add_custom_target(package_archive
    COMMAND ${CMAKE_COMMAND} -E tar cf "${PACKAGE_ARCHIVE}" --format=zip "${PACKAGE_NAME}"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMENT "Creating ${PACKAGE_ARCHIVE}"
    VERBATIM)
  add_dependencies(package_archive package)
else()
  set(PACKAGE_ARCHIVE "${CMAKE_BINARY_DIR}/${PACKAGE_NAME}.tar.gz")
  add_custom_target(package_archive
    COMMAND ${CMAKE_COMMAND} -E tar cf "${PACKAGE_ARCHIVE}" --format=gnutar "${PACKAGE_NAME}"
    COMMAND ${CMAKE_COMMAND} -E tar xf "${PACKAGE_ARCHIVE}"
    COMMAND ${CMAKE_COMMAND} -E tar czf "${PACKAGE_ARCHIVE}" "${PACKAGE_NAME}"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMENT "Creating ${PACKAGE_ARCHIVE}"
    VERBATIM)
  add_dependencies(package_archive package)
endif()
