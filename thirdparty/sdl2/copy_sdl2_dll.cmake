if(WIN32)
    message(STATUS "Running post-install script to find and install SDL3 DLL...")

    if(NOT DEFINED CMAKE_INSTALL_BINDIR)
        set(CMAKE_INSTALL_BINDIR "bin")
    endif()

    file(GLOB_RECURSE SDL3_DLL_FILES "${CMAKE_BINARY_DIR}/_deps/sdl3-build/sdl3*.dll")

    list(LENGTH SDL3_DLL_FILES SDL3_DLL_COUNT)
    if(SDL3_DLL_COUNT GREATER 0)
        list(GET SDL3_DLL_FILES 0 SDL3_DLL_PATH)
        message(STATUS "Found SDL3 DLL: ${SDL3_DLL_PATH}")
        file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}" TYPE FILE FILES "${SDL3_DLL_PATH}")
    else()
        message(WARNING "SDL3.dll not found! Ensure SDL3 is built correctly.")
    endif()
endif()
