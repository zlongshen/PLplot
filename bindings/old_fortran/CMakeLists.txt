# bindings/fortran/CMakeLists.txt
### Process this file with cmake to produce Makefile
###
# Copyright (C) 2006 Andrew Ross
# Copyright (C) 2006-2016 Alan W. Irwin
#
# This file is part of PLplot.
#
# PLplot is free software; you can redistribute it and/or modify
# it under the terms of the GNU Library General Public License as published
# by the Free Software Foundation; version 2 of the License.
#
# PLplot is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public License
# along with PLplot; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA

if(ENABLE_fortran)

  # Set the include path
  include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/lib/qsastime
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    )

  ## Build C part of Fortran bindings
  if(WIN32_OR_CYGWIN)
    # Set the flags for the C compiler. The C stubs need to have the
    # correct case and calling convention for the Fortran compiler
    IF(TARGET_FORTRAN MATCHES "IVF")
      SET(DEFFILE "_ifort")
      SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /DIVF")
    ELSEIF(TARGET_FORTRAN MATCHES "CVF")
      SET(DEFFILE "")
      SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /DCVF")
    ENDIF(TARGET_FORTRAN MATCHES "IVF")
  endif(WIN32_OR_CYGWIN)

  set(plplotfortranc_LIB_SRCS
    sc3d.c
    sccont.c
    scstubs.c
    )

  add_library(plplotfortranc ${plplotfortranc_LIB_SRCS})

  if(NON_TRANSITIVE)
    target_link_libraries(plplotfortranc PRIVATE plplot)
  else(NON_TRANSITIVE)
    target_link_libraries(plplotfortranc PUBLIC plplot)
  endif(NON_TRANSITIVE)

  if(USE_RPATH)
    get_target_property(LIB_INSTALL_RPATH plplot INSTALL_RPATH)
  endif(USE_RPATH)
  set_library_properties(plplotfortranc)

  install(TARGETS plplotfortranc
    EXPORT export_plplot
    ARCHIVE DESTINATION ${LIB_DIR}
    LIBRARY DESTINATION ${LIB_DIR}
    RUNTIME DESTINATION ${BIN_DIR}
    )

  # Build fortran part of Fortran bindings.
  # Notes:

  # The CMake Fortran dependency scanner finds the Fortran include
  # files, sfstubs.inc and plplot_parameters.inc and deals properly
  # with the dependencies on those files.  Therefore, there is no need
  # to include those files in the SRCS list or do anything special
  # with their dependencies.

  # Check consistency of plplot_parameters.inc.
  add_custom_target(
    check_fortran_parameters
    ${CMAKE_COMMAND} -E echo "Check that bindings/fortran/plplot_parameters.inc is consistent with the #defines in bindings/swig-support/plplotcapi.i"
    COMMAND ${CMAKE_COMMAND} -E remove -f
    ${CMAKE_CURRENT_BINARY_DIR}/plplot_parameters.inc_compare
    COMMAND
    sed -f ${CMAKE_CURRENT_SOURCE_DIR}/global_defines.sed <
    ${CMAKE_SOURCE_DIR}/bindings/swig-support/plplotcapi.i >
    ${CMAKE_CURRENT_BINARY_DIR}/plplot_parameters.inc_compare
    COMMAND
    COMMAND
    cmp ${CMAKE_CURRENT_SOURCE_DIR}/plplot_parameters.inc
    ${CMAKE_CURRENT_BINARY_DIR}/plplot_parameters.inc_compare
    )

  add_dependencies(check_all check_fortran_parameters)

  # Configure the Fortran source file that specifies the
  # types used for PLplot.
  if(PL_DOUBLE)
    set(ONE_IN_DEFAULT_PRECISION "1.0d0")
  else(PL_DOUBLE)
    set(ONE_IN_DEFAULT_PRECISION "1.0")
  endif(PL_DOUBLE)
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/plplot_types.f90.in
    ${CMAKE_CURRENT_BINARY_DIR}/plplot_types.f90
    @ONLY
    )

  set(plplotfortran_LIB_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/plplot_types.f90
    strutil.f90
    plparseopts.f90
    sfstubsfortran.f90
    )

  message(STATUS "MINGW = ${MINGW}")
  message(STATUS "MSYS  = ${MSYS}")
  if(WIN32_AND_NOT_CYGWIN AND BUILD_SHARED_LIBS AND NOT MINGW AND NOT MSYS)
    SET(plplotfortran_LIB_SRCS ${plplotfortran_LIB_SRCS} plplotfortran${DEFFILE}.def)
  endif(WIN32_AND_NOT_CYGWIN AND BUILD_SHARED_LIBS AND NOT MINGW AND NOT MSYS)

  add_library(plplotfortran ${plplotfortran_LIB_SRCS})

  if(NON_TRANSITIVE)
    target_link_libraries(plplotfortran PRIVATE plplotfortranc)
  else(NON_TRANSITIVE)
    target_link_libraries(plplotfortran PUBLIC plplotfortranc)
  endif(NON_TRANSITIVE)

  set_library_properties(plplotfortran)

  install(TARGETS plplotfortran
    EXPORT export_plplot
    ARCHIVE DESTINATION ${LIB_DIR}
    LIBRARY DESTINATION ${LIB_DIR}
    RUNTIME DESTINATION ${BIN_DIR}
    )

  # For CMake-2.6.0 and above, the module files are created by
  # default during the library build in the ${CMAKE_CURRENT_BINARY_DIR} directory.
  install(
    FILES
    ${CMAKE_CURRENT_BINARY_DIR}/plplot.mod
    ${CMAKE_CURRENT_BINARY_DIR}/plplotp.mod
    ${CMAKE_CURRENT_BINARY_DIR}/plplot_types.mod
    ${CMAKE_CURRENT_BINARY_DIR}/plplot_graphics.mod
    ${CMAKE_CURRENT_BINARY_DIR}/plplot_str.mod
    ${CMAKE_CURRENT_BINARY_DIR}/plplot_strutils.mod
    DESTINATION ${FORTRAN_MOD_DIR}
    )

  # Configure pkg-config *.pc file corresponding to libplplotfortran
  # Note, Fortran executables need to link plplotfortranc as a public
  # library so specify it as 7th argument.
  pkg_config_file("fortran" "Fortran" " Fortran binding" "plplotfortran" "-I\"${FORTRAN_MOD_DIR}\"" "-lplplot" "-lplplotfortranc")
endif(ENABLE_fortran)
