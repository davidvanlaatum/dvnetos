function(handleArchAlias var)
    if (${${var}} STREQUAL "arm64")
        set(${var} "aarch64" PARENT_SCOPE)
    endif ()
endfunction()
