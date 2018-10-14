# - Find Tango
# This module finds an installed Tango client C-API package.
#
# It sets the following variables:
#  Tango_FOUND       - Set to false, or undefined, if Tango isn't found.
#  Tango_INCLUDE_DIRS - The Tango include directory.
#  Tango_LIBRARIES     - The Tango library to link against.

SET(Tango_INCLUDE_DIR ${Tango_DIR}/include)
SET(Tango_LIBRARY ${Tango_DIR}/${ANDROID_ABI}/libtango_client_api.so)
SET(Tango_support_LIBRARY ${Tango_DIR}/${ANDROID_ABI}/libtango_support_api.so)

IF (Tango_INCLUDE_DIR AND Tango_LIBRARY AND Tango_support_LIBRARY)
   SET(Tango_FOUND TRUE)
   SET(Tango_INCLUDE_DIRS ${Tango_INCLUDE_DIR})
   SET(Tango_LIBRARIES ${Tango_LIBRARY} ${Tango_support_LIBRARY})
ENDIF (Tango_INCLUDE_DIR AND Tango_LIBRARY AND Tango_support_LIBRARY)

IF (Tango_FOUND)	
   # show which Tango was found only if not quiet
   IF (NOT Tango_FIND_QUIETLY)
      MESSAGE(STATUS "Found Tango: ${Tango_INCLUDE_DIRS}")
   ENDIF (NOT Tango_FIND_QUIETLY)
ELSE (Tango_FOUND)
   # fatal error if Tango is required but not found
   IF (Tango_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could not find Tango (client and/or support libraries)")
   ENDIF (Tango_FIND_REQUIRED)
ENDIF (Tango_FOUND)

