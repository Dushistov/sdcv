project(sdcv)

# Older versions have a different signature for CMAKE_MINIMUM_REQUIRED,
# check it manually just to make sure
if (${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} LESS 2.8)
  message(FATAL_ERROR "${PROJECT_NAME} requires at least CMake v2.8."
		" You are running v${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}."
		" Please upgrade." )
endif(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} LESS 2.8)

# If we get this far, use the modern signature.  This will also cause newer
# CMake versions to try to be backwards-compatible with the desired version
cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
cmake_policy(VERSION 2.8)

set(ZLIB_FIND_REQUIRED True)
include(FindZLIB)

set(GLIB2_REQ "'glib-2.0 >= 2.6.1'")
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindGLIB2.cmake")

if (NOT GLIB2_FOUND)
	message(FATAL_ERROR "sdcv require ${GLIB2_REQ}, "
		"make sure that you install it")
endif()

find_path(READLINE_INCLUDE_DIR readline/readline.h)
find_library(READLINE_LIBRARY NAMES readline) 
if (READLINE_INCLUDE_DIR AND READLINE_LIBRARY)
  set(WITH_READLINE TRUE)
endif ()

find_package(GettextTools REQUIRED)
set(gettext_stockDir "${CMAKE_CURRENT_SOURCE_DIR}/po")
set(gettext_langDir  "${CMAKE_CURRENT_BINARY_DIR}/lang")
set(gettext_outDir   "${CMAKE_CURRENT_BINARY_DIR}/locale")

set(sdcv_SRCS
  src/sdcv.cpp
  src/readline.cpp
  src/readline.hpp
  src/libwrapper.cpp 
  src/libwrapper.hpp
  src/utils.cpp 
  src/utils.hpp

  src/lib/lib.cpp
  src/lib/lib.h
  src/lib/dictziplib.cpp
  src/lib/dictziplib.hpp  
  src/lib/distance.cpp 
  src/lib/distance.h
  src/lib/mapfile.hpp
  src/lib/file.hpp
)

gettext_make_target("lang"
  HIERARCHY "{1}/{2}/{3}/{4}.mo"
  KEYWORDS  "_"
  DOMAIN    "sdcv"
  STOCK_DIR ${gettext_stockDir}
  LANG_DIR  ${gettext_langDir}
  OUT_DIR   ${gettext_outDir}
  SOURCE    ${sdcv_SRCS})

list(APPEND makeCleanFiles ${gettext_outDir})

include(CheckFunctionExists)
check_function_exists(mmap HAVE_MMAP)

include(CheckIncludeFile)
check_include_file(locale.h HAVE_LOCALE_H)

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake
	${CMAKE_CURRENT_BINARY_DIR}/config.h)


include_directories(
	${ZLIB_INCLUDE_DIR}
	${GLIB2_INCLUDE_DIRS}
	${CMAKE_CURRENT_SOURCE_DIR}/src/lib
	${CMAKE_CURRENT_BINARY_DIR}
)

#
# Packing stuff
#
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "console version of StarDict program")
set(CPACK_PACKAGE_VENDOR "Evgeniy Dushistov <dushistov@mail.ru>")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README")
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "5")
set(CPACK_PACKAGE_VERSION_PATCH "0-beta1")

set(sdcv_VERSION
	"${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")

add_definitions(-DVERSION="${sdcv_VERSION}" -DHAVE_CONFIG_H)

add_executable(sdcv ${sdcv_SRCS})

target_link_libraries(sdcv
  ${GLIB2_LIBRARIES}
  ${ZLIB_LIBRARIES}
  ${READLINE_LIBRARY}
)
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "locale")

include(CPack)

option(BUILD_TESTS "Enable automatic testing" False)
if (BUILD_TESTS)
  find_program(SHELL_CMD NAMES sh bash
    DOC "Shell scripts interpretator command")
  message(STATUS "Build tests")
  enable_testing()# Force "make test" to works

  get_target_property(sdcv_path sdcv LOCATION)
  message(STATUS "path ${sdcv_path}")
  string(REGEX REPLACE "\\$\\(.*\\)" "\${CTEST_CONFIGURATION_TYPE}"
    sdcv_path "${sdcv_path}")

  macro(add_sdcv_shell_test test_name)
    add_test(${test_name} "${SHELL_CMD}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/${test_name}" "${sdcv_path}" "${CMAKE_CURRENT_SOURCE_DIR}/tests")
  endmacro()

  add_sdcv_shell_test(t_list)
  add_sdcv_shell_test(t_use)
  add_sdcv_shell_test(t_interactive)
  add_sdcv_shell_test(t_utf8output)
  add_sdcv_shell_test(t_utf8input)
  add_sdcv_shell_test(t_datadir)

endif ()