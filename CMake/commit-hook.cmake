function(write_commit_hook)
  if (EXISTS ${CMAKE_SOURCE_DIR}/.git/hooks)
    file(WRITE ${CMAKE_SOURCE_DIR}/.git/hooks/pre-commit
        "#!/bin/sh\n"
        "set -e\n"
        "for i in */CMakeCache.txt\n"
        "do\n"
        "  cmake --build $(dirname $i) --target all\n"
        "  cmake --build $(dirname $i) --target test\n"
        "done\n"
    )
    file(CHMOD ${CMAKE_SOURCE_DIR}/.git/hooks/pre-commit PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
  endif ()
endfunction()
