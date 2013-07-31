cmake_policy(SET CMP0007 NEW)

# List of dependencies (mostly these are build-tools) which should be
# ignored.
set(ignored_dependencies_LIST
  # build tools
  pkg-config
  bison
  flex
  python2-devel
  # dependencies which we want to ignore
  # (none so far).
)

file(STRINGS ${FILENAME} lines)
list(LENGTH lines nlines)
# There are currently groups of 9 lines for each
# gtk software package specifying the required
# build-configuration information.
set(group_size 12)
set(remove_indices 0 1 2 3 4 5 6 7 8 9 10 11)
math(EXPR max_index "(${nlines}/${group_size}) - 1")
foreach(index RANGE 0 ${max_index})
  list(GET lines 0 package)
  list(GET lines 1 config_type)
  list(GET lines 2 supports_non_srcdir_builds)
  list(GET lines 3 supports_parallel_builds)
  list(GET lines 4 config_arguments)
  list(GET lines 5 make_arguments)
  list(GET lines 6 download_hash_type)
  list(GET lines 7 download_hash)
  list(GET lines 8 download_href)
  list(GET lines 9 dependencies)
  list(GET lines 10 suggests)
  list(GET lines 11 after)
  list(REMOVE_AT lines ${remove_indices})

  if(supports_non_srcdir_builds)
    set(BUILD_IN_SOURCE OFF)
  else(supports_non_srcdir_builds)
    set(BUILD_IN_SOURCE ON)
  endif(supports_non_srcdir_builds)

  if(supports_parallel_builds)
    set(BUILD_COMMAND BP_PARALLEL_MAKE_COMMAND)
  else(supports_parallel_builds)
    set(BUILD_COMMAND BP_MAKE_COMMAND)
  endif(supports_parallel_builds)

  # Extra CFLAGS fixups for individual packages.
  if(package STREQUAL "glib")
    set(MSYS_EXTRA_CFLAGS "-march=native ")
    set(UNIX_EXTRA_CFLAGS)
  else(package STREQUAL "glib")
    set(MSYS_EXTRA_CFLAGS)
    set(UNIX_EXTRA_CFLAGS)
  endif(package STREQUAL "glib")

  # Turn colon-separated lists into official CMake lists.
  string(REGEX REPLACE ":" ";" dependencies "${dependencies}")
  string(REGEX REPLACE ":" ";" suggests "${suggests}")
  string(REGEX REPLACE ":" ";" after "${after}")

  # For now, ignore "after" and treat "dependencies" and "suggests" the same
  if(suggests)
    list(APPEND dependencies "${suggests}")
  endif(suggests)

  # Configure the package file using the appropriate template file.
  if(config_type STREQUAL "autotools" OR config_type STREQUAL "tarball" OR config_type STREQUAL "cmake")
    set(template_file template.bp.in)
  else(config_type STREQUAL "autotools" OR config_type STREQUAL "tarball" OR config_type STREQUAL "cmake")
    message(FATAL_ERROR "incorrect config_type = ${config_type}")
  endif(config_type STREQUAL "autotools" OR config_type STREQUAL "tarball" OR config_type STREQUAL "cmake")
  
  if(NOT package)
    message(FATAL_ERROR "bad package = ${package}")
  endif(NOT package)
  
  file(MAKE_DIRECTORY ${package})
  configure_file(${template_file} ${package}/bp.cmake @ONLY)
  
endforeach(index RANGE 0 ${max_index})
