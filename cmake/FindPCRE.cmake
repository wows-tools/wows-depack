# Look for the PCRE library and headers

find_path(PCRE_INCLUDE_DIR pcre.h)
find_library(PCRE_LIBRARY NAMES pcre pcre16 pcre32)

# Check if the library and headers were found

if(PCRE_INCLUDE_DIR AND PCRE_LIBRARY)
    set(PCRE_FOUND TRUE)
endif()

# Report the results

if(PCRE_FOUND)
    message(STATUS "Found PCRE: ${PCRE_INCLUDE_DIR}, ${PCRE_LIBRARY}")
else()
    message(STATUS "Could not find PCRE")
endif()

# Define the PCRE target

if(PCRE_FOUND)
    add_library(PCRE::PCRE INTERFACE IMPORTED)
    target_include_directories(PCRE::PCRE INTERFACE ${PCRE_INCLUDE_DIR})
    target_link_libraries(PCRE::PCRE INTERFACE ${PCRE_LIBRARY})
endif()
