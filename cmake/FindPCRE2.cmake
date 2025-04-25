# Find PCRE2 library and include paths
# PCRE2_INCLUDE_DIRS - where to find pcre2.h
# PCRE2_LIBRARIES - List of libraries when using PCRE2
# PCRE2_FOUND - True if PCRE2 found

find_path(PCRE2_INCLUDE_DIRS NAMES pcre2.h)
find_library(PCRE2_LIBRARY NAMES pcre2-8)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PCRE2 DEFAULT_MSG PCRE2_LIBRARY PCRE2_INCLUDE_DIRS)

mark_as_advanced(PCRE2_INCLUDE_DIRS PCRE2_LIBRARY)

if(PCRE2_FOUND)
    set(PCRE2_LIBRARIES ${PCRE2_LIBRARY})
endif() 