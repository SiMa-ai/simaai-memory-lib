add_library(simaai-memory-lib::simaaimem SHARED IMPORTED)

set_target_properties(simaai-memory-lib::simaaimem PROPERTIES
    IMPORTED_LOCATION "/usr/lib/aarch64-linux-gnu/libsimaaimem.so"
    INTERFACE_INCLUDE_DIRECTORIES "/usr/include"
)

