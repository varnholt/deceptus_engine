if(WIN32)
    message(STATUS "Running post-install script to find and install SDL2 DLL...")

    if(NOT DEFINED CMAKE_INSTALL_BINDIR)
        set(CMAKE_INSTALL_BINDIR "bin")
    endif()

    file(GLOB_RECURSE SDL2_DLL_FILES "${CMAKE_BINARY_DIR}/_deps/sdl2-build/sdl2*.dll")

    list(LENGTH SDL2_DLL_FILES SDL2_DLL_COUNT)
    if(SDL2_DLL_COUNT GREATER 0)
        list(GET SDL2_DLL_FILES 0 SDL2_DLL_PATH)
        message(STATUS "Found SDL2 DLL: ${SDL2_DLL_PATH}")
        file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}" TYPE FILE FILES "${SDL2_DLL_PATH}")
    else()
        message(WARNING "SDL2.dll not found! Ensure SDL2 is built correctly.")
    endif()
endif()
