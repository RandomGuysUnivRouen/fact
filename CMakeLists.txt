cmake_minimum_required (VERSION 2.6)
cmake_policy (VERSION 2.6)

project(fact)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

set (DATADIR "${CMAKE_INSTALL_PREFIX}/share")
set (PKGDATADIR "${DATADIR}/fact")
set (GETTEXT_PACKAGE "fact")
set (RELEASE_NAME "Alpha")
set (VERSION "0.1")
set (VERSION_INFO "Release")
set (CMAKE_C_FLAGS "-ggdb")
set (PREFIX ${CMAKE_INSTALL_PREFIX})
set (DOLLAR "$")
set(CMAKE_MAN_DIR "man" CACHE STRING
  "Install location for man pages (relative to prefix).")

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
# Mac OS X specific code
SET(OperatingSystem "Mac OS X")
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

add_subdirectory (src)
add_subdirectory (doc)
