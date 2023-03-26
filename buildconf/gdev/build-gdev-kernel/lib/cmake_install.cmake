# Install script for directory: /home/b/2.5bay/mthesis-unsync/projects/trusted-periph/src/gpu/gdev-guest/lib

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/home/b/2.5bay/mthesis-unsync/projects/trusted-periph/buildconf/gdev/install")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "0")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "TRUE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/home/b/2.5bay/mthesis-unsync/projects/trusted-periph/output/buildroot-linux-cca-guest/host/bin/aarch64-none-linux-gnu-objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  foreach(file
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/gdev/lib64/libgdev.so.1.0.0"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/gdev/lib64/libgdev.so.1"
      )
    if(EXISTS "${file}" AND
       NOT IS_SYMLINK "${file}")
      file(RPATH_CHECK
           FILE "${file}"
           RPATH "")
    endif()
  endforeach()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/gdev/lib64" TYPE SHARED_LIBRARY FILES
    "/home/b/2.5bay/mthesis-unsync/projects/trusted-periph/buildconf/gdev/build-gdev-kernel/lib/libgdev.so.1.0.0"
    "/home/b/2.5bay/mthesis-unsync/projects/trusted-periph/buildconf/gdev/build-gdev-kernel/lib/libgdev.so.1"
    )
  foreach(file
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/gdev/lib64/libgdev.so.1.0.0"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/gdev/lib64/libgdev.so.1"
      )
    if(EXISTS "${file}" AND
       NOT IS_SYMLINK "${file}")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/home/b/2.5bay/mthesis-unsync/projects/trusted-periph/output/buildroot-linux-cca-guest/host/bin/aarch64-none-linux-gnu-strip" "${file}")
      endif()
    endif()
  endforeach()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/gdev/lib64/libgdev.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/gdev/lib64/libgdev.so")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/gdev/lib64/libgdev.so"
         RPATH "")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/gdev/lib64" TYPE SHARED_LIBRARY FILES "/home/b/2.5bay/mthesis-unsync/projects/trusted-periph/buildconf/gdev/build-gdev-kernel/lib/libgdev.so")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/gdev/lib64/libgdev.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/gdev/lib64/libgdev.so")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/home/b/2.5bay/mthesis-unsync/projects/trusted-periph/output/buildroot-linux-cca-guest/host/bin/aarch64-none-linux-gnu-strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/gdev/lib64/libgdev.so")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/b/2.5bay/mthesis-unsync/projects/trusted-periph/buildconf/gdev/build-gdev-kernel/lib/libgdev_static.a")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/gdev/include" TYPE FILE FILES
    "/home/b/2.5bay/mthesis-unsync/projects/trusted-periph/src/gpu/gdev-guest/common/gdev_api.h"
    "/home/b/2.5bay/mthesis-unsync/projects/trusted-periph/src/gpu/gdev-guest/common/gdev_nvidia_def.h"
    "/home/b/2.5bay/mthesis-unsync/projects/trusted-periph/src/gpu/gdev-guest/common/gdev_list.h"
    "/home/b/2.5bay/mthesis-unsync/projects/trusted-periph/src/gpu/gdev-guest/common/gdev_time.h"
    "/home/b/2.5bay/mthesis-unsync/projects/trusted-periph/buildconf/gdev/build-gdev-kernel/gdev_autogen.h"
    )
endif()

