find_path(Sodium_INCLUDE_DIR sodium.h)
find_library(Sodium_LIBRARY NAMES sodium libsodium)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    Sodium
    REQUIRED_VARS Sodium_LIBRARY Sodium_INCLUDE_DIR
)

if(Sodium_FOUND AND NOT TARGET Sodium::Sodium)
    add_library(Sodium::Sodium IMPORTED INTERFACE)
    set_target_properties(
        Sodium::Sodium
        PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${Sodium_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES "${Sodium_LIBRARY}"
    )
endif()
