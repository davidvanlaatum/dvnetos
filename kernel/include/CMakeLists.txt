if (TARGET kernel)
    target_sources(kernel PRIVATE
            alloca.h
            cstdarg
            cstddef
            cstdint
            cstdio
            cstring
            limine.h
            stdarg.h
            stddef.h
            stdint.h
            stdio.h
            string.h
    )
endif ()
