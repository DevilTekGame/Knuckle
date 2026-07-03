# Auto-download CEF binary distribution if not found.
# Set CEF_VERSION to pin a specific version (e.g. "149.0.6+g0d0eeb6+chromium-149.0.7827.201")
# Set CEF_ROOT to skip download and use a local copy.

if(CEF_ROOT AND IS_DIRECTORY "${CEF_ROOT}")
  message(STATUS "Using existing CEF at ${CEF_ROOT}")
  return()
endif()

# Check project root for an already-extracted cef_binary_* folder.
file(GLOB LOCAL_CEF_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/cef_binary_*")
if(LOCAL_CEF_DIRS)
  list(GET LOCAL_CEF_DIRS 0 CEF_ROOT)
  set(CEF_ROOT "${CEF_ROOT}" CACHE PATH "Path to CEF binary distribution" FORCE)
  message(STATUS "Using local CEF at ${CEF_ROOT}")
  return()
endif()

set(CEF_VERSION "149.0.6+g0d0eeb6+chromium-149.0.7827.201" CACHE STRING "CEF binary distribution version")

if(WIN32)
  set(CEF_PLATFORM "windows64")
  set(CEF_EXT "zip")
elseif(APPLE)
  set(CEF_PLATFORM "macosx64")
  set(CEF_EXT "tar.bz2")
else()
  set(CEF_PLATFORM "linux64")
  set(CEF_EXT "tar.bz2")
endif()

string(REPLACE "+" "%2B" CEF_VERSION_URL "${CEF_VERSION}")
set(CEF_FILENAME "cef_binary_${CEF_VERSION}_${CEF_PLATFORM}.${CEF_EXT}")
set(CEF_URL "https://cef-builds.spotifycdn.com/${CEF_FILENAME}")

set(CEF_DOWNLOAD_DIR "${CMAKE_CURRENT_BINARY_DIR}/cef_download")
set(CEF_ARCHIVE "${CEF_DOWNLOAD_DIR}/${CEF_FILENAME}")

if(NOT EXISTS "${CEF_ARCHIVE}")
  message(STATUS "Downloading CEF ${CEF_VERSION} for ${CEF_PLATFORM}...")
  file(DOWNLOAD "${CEF_URL}" "${CEF_ARCHIVE}"
    SHOW_PROGRESS
    STATUS CEF_DOWNLOAD_STATUS
    LOG CEF_DOWNLOAD_LOG)
  list(GET CEF_DOWNLOAD_STATUS 0 CEF_DOWNLOAD_CODE)
  if(NOT CEF_DOWNLOAD_CODE EQUAL 0)
    message(FATAL_ERROR "CEF download failed: ${CEF_DOWNLOAD_LOG}")
  endif()
endif()

message(STATUS "Extracting CEF...")
if(CEF_EXT STREQUAL "zip")
  file(ARCHIVE_EXTRACT INPUT "${CEF_ARCHIVE}" DESTINATION "${CEF_DOWNLOAD_DIR}")
else()
  execute_process(COMMAND tar -xjf "${CEF_ARCHIVE}"
    WORKING_DIRECTORY "${CEF_DOWNLOAD_DIR}"
    RESULT_VARIABLE TAR_RESULT)
  if(NOT TAR_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to extract ${CEF_ARCHIVE}")
  endif()
endif()

file(GLOB CEF_EXTRACTED_DIR "${CEF_DOWNLOAD_DIR}/cef_binary_${CEF_VERSION}_${CEF_PLATFORM}")
if(NOT CEF_EXTRACTED_DIR)
  file(GLOB CEF_EXTRACTED_DIR "${CEF_DOWNLOAD_DIR}/cef_binary_*")
endif()

if(NOT CEF_EXTRACTED_DIR)
  message(FATAL_ERROR "Could not find extracted CEF directory in ${CEF_DOWNLOAD_DIR}")
endif()

list(GET CEF_EXTRACTED_DIR 0 CEF_ROOT)
set(CEF_ROOT "${CEF_ROOT}" CACHE PATH "Path to CEF binary distribution" FORCE)
message(STATUS "CEF downloaded and extracted to ${CEF_ROOT}")
