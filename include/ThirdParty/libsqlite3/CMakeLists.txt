# Copyright (C) 2007-2012 LuaDist.
# Created by Peter Kapec
# Redistribution and use of this file is allowed according to the terms of the MIT license.
# For details see the COPYRIGHT file distributed with LuaDist.
# Please note that the package source code is licensed under its own license.

# Project
#project ( sqlite3 C )
#cmake_minimum_required ( VERSION 2.8 )
#include ( cmake/dist.cmake )

# SQLite configuration
find_package ( Threads )
if ( Threads_FOUND )
  if ( CMAKE_USE_PTHREADS_INIT )
    add_definitions ( -D_REENTRANT=1 -DTHREADSAFE=1 )
    set ( EXTRA_LIBS ${CMAKE_THREAD_LIBS_INIT} )
  endif ( )
endif ( )
find_package ( Readline )
if ( READLINE_FOUND )
  include_directories ( ${READLINE_INCLUDE_DIRS} )
endif ( )

# Add dl on unix systems
if ( UNIX )
  set ( EXTRA_LIBS ${EXTRA_LIBS} dl )
endif ( )

# SQLite Library
add_library ( sqlite3 STATIC sqlite3.c )
if ( CMAKE_USE_PTHREADS_INIT )
  target_link_libraries ( sqlite3 ${EXTRA_LIBS} )
endif ( )
#install_library ( sqlite3 )

# Install Headers
#install_header ( sqlite3.h sqlite3ext.h )