# - Finds GNU gettext and provides tools
# This module looks for the GNU gettext tools. This module defines the
# following values:
#  GETTEXT_XGETTEXT_EXECUTABLE: The full path to the xgettext tool.
#  GETTEXT_MSGMERGE_EXECUTABLE: The full path to the msgmerge tool.
#  GETTEXT_MSGFMT_EXECUTABLE:   The full path to the msgfmt tool.
#  GETTEXT_FOUND: True if gettext has been found.
#  GETTEXT_VERSION_STRING: The version of gettext found (since CMake 2.8.8)
#
# It provides the following macro:
#
# GETTEXT_MAKE_TARGET (
#       targetName
#       HIERARCHY <HIERARCHY_FORMAT>
#       KEYWORDS  keyword1 ... keywordN
#       DOMAIN    <TRANSLATION_DOMAIN>
#       STOCK_DIR <DIR>
#       LANG_DIR  <DIR>
#       OUT_DIR   <DIR>
#       SOURCE    sourceFile1 ... sourceFileN )
#
#    Creates a target that will take a set of translatable source files,
#    create a Gettext pot file then copy stock translations in to the build
#    directory to allow user editing, then compiles them in to mo files in a
#    directory hierarchy to be used in the application.
#
#   USAGE:
#      targetName (e.g., "lang")
#        The name of the target that will be created to generate translations.
#
#      HIERARCHY (e.g., "{1}/{2}/{3}/{4}.mo")
#        This is the format in which compiled message catalogs are placed.
#        {1}: The path prefix.      (e.g., "/my-repo/build/locale/")
#        {2}: The language name.    (e.g., "en")
#        {3}: The catalog category. (e.g., "LC_MESSAGES")
#        {4}: The domain.           (e.g., "my-app")
#
#      KEYWORDS (e.g., "_")
#        A list of keywords used by xgettext to find translatable strings in the
#        source files.
#
#      DOMAIN (e.g., "my-app")
#        The Gettext domain. It should be unique to your application.
#
#      STOCK_DIR (e.g., "/my-repo/stock-lang/")
#        The path to the initial translations to be copied to the LANG_DIR.
#        If you have a set of official translations in your source repository,
#        you'd want to set STOCK_DIR to this.
#
#      LANG_DIR (e.g., "lang")
#        The name of the directory to be created in the build folder, containing
#        editable translations and updated templates.
#
#      OUT_DIR (e.g., "locale")
#        The directory that compiled catalogs will be placed in, according to
#        the HIERARCHY format.
#
#      SOURCE (e.g., "main.c")
#        A list of source files to read translatable strings from. Usually this
#        could be the same list you pass to add_executable.
#
#      If you use the examples above and have a structure like this:
#        /my-repo/stock-lang/en.po
#
#      You may end up with this structure:
#        /my-repo/stock-lang/en.po
#        /my-repo/build/lang/my-app.pot
#        /my-repo/build/lang/en.po
#        /my-repo/build/locale/en/LC_MESSAGES/my-app.mo

# This nasty set of tools is divided up in to three files:
#   FindGettextTools.cmake
#     This is the file you're reading right now. It provides a neat macro.
#   FindGettextTools/config.cmake.in
#     This is used as the bridge to transfer arguments from the macro to the
#     actual script used to do Gettext stuff. A copy is created and filled in by
#     FindGettextTools.cmake and read by FindGettextTools/script.cmake.
#     The copy is found in the target's directory in the CMakeFiles directory
#     under the name 'gettext.cmake'.
#   FindGettextTools/script.cmake
#     Does Gettext things based on the bridge config file whenever the target
#     created using FindGettextTools.cmake is run.

FIND_PROGRAM(GETTEXT_XGETTEXT_EXECUTABLE xgettext)
FIND_PROGRAM(GETTEXT_MSGMERGE_EXECUTABLE msgmerge)
FIND_PROGRAM(GETTEXT_MSGFMT_EXECUTABLE   msgfmt)

SET(_gettextScript "${CMAKE_CURRENT_LIST_DIR}/FindGettextTools/script.cmake")
SET(_gettextConfig "${CMAKE_CURRENT_LIST_DIR}/FindGettextTools/config.cmake.in")

IF(GETTEXT_XGETTEXT_EXECUTABLE)
   EXECUTE_PROCESS(COMMAND ${GETTEXT_XGETTEXT_EXECUTABLE} --version
      OUTPUT_VARIABLE gettext_version
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE)
   IF(gettext_version MATCHES "^xgettext \\(.*\\) [0-9]")
      STRING(REGEX REPLACE "^xgettext \\([^\\)]*\\) ([0-9\\.]+[^ \n]*).*" "\\1"
         GETTEXT_VERSION_STRING "${gettext_version}")
   ENDIF()
   SET(gettext_version)
ENDIF()

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Gettext
   REQUIRED_VARS
      GETTEXT_XGETTEXT_EXECUTABLE
      GETTEXT_MSGMERGE_EXECUTABLE
      GETTEXT_MSGFMT_EXECUTABLE
   VERSION_VAR GETTEXT_VERSION_STRING)

INCLUDE(CMakeParseArguments)

FUNCTION(GETTEXT_MAKE_TARGET _targetName)
   SET(_oneValueArgs   HIERARCHY DOMAIN STOCK_DIR LANG_DIR OUT_DIR)
   SET(_multiValueArgs KEYWORDS SOURCE)
   
   CMAKE_PARSE_ARGUMENTS(_parsedArguments
      ""
      "${_oneValueArgs}"
      "${_multiValueArgs}"
      "${ARGN}")
   
   IF(NOT (
         _parsedArguments_HIERARCHY AND
         _parsedArguments_KEYWORDS  AND
         _parsedArguments_DOMAIN    AND
         _parsedArguments_STOCK_DIR AND
         _parsedArguments_LANG_DIR  AND
         _parsedArguments_OUT_DIR   AND
         _parsedArguments_SOURCE))
      MESSAGE(FATAL_ERROR "Wrong usage!")
   ENDIF()
   
   SET(_config
      "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_targetName}.dir/gettext.cmake")
   
   CONFIGURE_FILE(${_gettextConfig} ${_config})
   
   ADD_CUSTOM_TARGET(${_targetName} ${CMAKE_COMMAND} "-P" ${_gettextScript}
      ${_config})
ENDFUNCTION()

IF(GETTEXT_MSGMERGE_EXECUTABLE AND
   GETTEXT_MSGFMT_EXECUTABLE AND
   GETTEXT_XGETTEXT_EXECUTABLE)
   SET(GETTEXT_FOUND TRUE)
ELSE()
   SET(GETTEXT_FOUND FALSE)
   IF(GETTEXT_REQUIRED)
      MESSAGE(FATAL_ERROR "Gettext not found")
   ENDIF()
ENDIF()
