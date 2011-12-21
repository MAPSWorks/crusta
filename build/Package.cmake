# Setup package name, version and description
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})

set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_MAJOR_VERSION})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_MINOR_VERSION})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_PATCH_VERSION})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})

set(CPACK_PACKAGE_VENDOR "keckcaves.org")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "A geologist's virtual globe to support remote virtual studies.")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/readme.txt")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/license.txt")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/readme.txt")


# Setup the system name
if (CMAKE_SIZEOF_VOID_P EQUAL 4)
  set(CPACK_SYSTEM_NAME Linux-i386)
else()
  set(CPACK_SYSTEM_NAME Linux-amd64)
endif()


# Setup the generators
set(CPACK_GENERATOR DEB;RPM)

if (CMAKE_SIZEOF_VOID_P EQUAL 4)
  set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE i386)
  set(CPACK_RPM_PACKAGE_ARCHITECTURE i386)
else()
  set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE amd64)
  set(CPACK_RPM_PACKAGE_ARCHITECTURE x86_64)
endif()

# Setup debian specific properties
set(CPACK_DEBIAN_PACKAGE_MAINTAINER tbernardin@ucdavis.edu)
set(CPACK_DEBIAN_PACKAGE_SECTION science)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libgdal1-1.7.0 | libgdal1-1.6.0, vrui (>= 2.2)")
#file(READ ${CPACK_PACKAGE_DESCRIPTION_FILE} CPACK_DEBIAN_PACKAGE_DESCRIPTION)


# Include the Cpack module
include(CPack)
