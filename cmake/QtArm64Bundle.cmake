function(qtarm64_bundle_app target)
    if(NOT TARGET ${target})
        message(FATAL_ERROR "qtarm64_bundle_app(): target '${target}' does not exist")
    endif()

    include(GNUInstallDirs)
    install(TARGETS ${target}
        BUNDLE  DESTINATION .
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )

    # Qt 6.3+; Linux deployment is supported, and the script creates qt.conf plus
    # the Qt libraries/plugins needed by the executable.
    qt_generate_deploy_app_script(
        TARGET ${target}
        FILENAME_VARIABLE deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT "${deploy_script}")
endfunction()