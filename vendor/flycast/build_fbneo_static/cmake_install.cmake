# Install script for directory: C:/msys64/home/24946/fbneowrok/yizhi/fbneo/vendor/flycast

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/flycast_libretro")
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

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "C:/TDM-GCC-64/bin/objdump.exe")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/msys64/home/24946/fbneowrok/yizhi/fbneo/vendor/flycast/build_fbneo_static/core/deps/xxHash/cmake_unofficial/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/msys64/home/24946/fbneowrok/yizhi/fbneo/vendor/flycast/build_fbneo_static/core/deps/glm/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/msys64/home/24946/fbneowrok/yizhi/fbneo/vendor/flycast/build_fbneo_static/core/deps/miniupnpc/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/msys64/home/24946/fbneowrok/yizhi/fbneo/vendor/flycast/build_fbneo_static/core/deps/libelf/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/msys64/home/24946/fbneowrok/yizhi/fbneo/vendor/flycast/build_fbneo_static/core/archive/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/msys64/home/24946/fbneowrok/yizhi/fbneo/vendor/flycast/build_fbneo_static/core/cfg/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/msys64/home/24946/fbneowrok/yizhi/fbneo/vendor/flycast/build_fbneo_static/core/hw/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/msys64/home/24946/fbneowrok/yizhi/fbneo/vendor/flycast/build_fbneo_static/core/imgread/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/msys64/home/24946/fbneowrok/yizhi/fbneo/vendor/flycast/build_fbneo_static/core/input/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/msys64/home/24946/fbneowrok/yizhi/fbneo/vendor/flycast/build_fbneo_static/core/log/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/msys64/home/24946/fbneowrok/yizhi/fbneo/vendor/flycast/build_fbneo_static/core/network/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/msys64/home/24946/fbneowrok/yizhi/fbneo/vendor/flycast/build_fbneo_static/core/oslib/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/msys64/home/24946/fbneowrok/yizhi/fbneo/vendor/flycast/build_fbneo_static/core/audio/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/msys64/home/24946/fbneowrok/yizhi/fbneo/vendor/flycast/build_fbneo_static/core/reios/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/msys64/home/24946/fbneowrok/yizhi/fbneo/vendor/flycast/build_fbneo_static/core/achievements/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/msys64/home/24946/fbneowrok/yizhi/fbneo/vendor/flycast/build_fbneo_static/core/ui/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/msys64/home/24946/fbneowrok/yizhi/fbneo/vendor/flycast/build_fbneo_static/core/lua/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/msys64/home/24946/fbneowrok/yizhi/fbneo/vendor/flycast/build_fbneo_static/core/profiler/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/msys64/home/24946/fbneowrok/yizhi/fbneo/vendor/flycast/build_fbneo_static/core/wsi/cmake_install.cmake")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/libretro" TYPE STATIC_LIBRARY FILES "C:/msys64/home/24946/fbneowrok/yizhi/fbneo/vendor/flycast/build_fbneo_static/libflycast_libretro.a")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  include("C:/msys64/home/24946/fbneowrok/yizhi/fbneo/vendor/flycast/build_fbneo_static/CMakeFiles/flycast_libretro.dir/install-cxx-module-bmi-Release.cmake" OPTIONAL)
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "C:/msys64/home/24946/fbneowrok/yizhi/fbneo/vendor/flycast/build_fbneo_static/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
if(CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_COMPONENT MATCHES "^[a-zA-Z0-9_.+-]+$")
    set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
  else()
    string(MD5 CMAKE_INST_COMP_HASH "${CMAKE_INSTALL_COMPONENT}")
    set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INST_COMP_HASH}.txt")
    unset(CMAKE_INST_COMP_HASH)
  endif()
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "C:/msys64/home/24946/fbneowrok/yizhi/fbneo/vendor/flycast/build_fbneo_static/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
