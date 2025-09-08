cmake_minimum_required(VERSION 3.23)
set(APPNAME RingBuffer)
project(${APPNAME} VERSION 0.1.0)

option(RINGBUFFER_INSTALL "Generate install target of the library" OFF)

add_library(${APPNAME} INTERFACE)

set_target_properties(${APPNAME} PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED YES
)

include(GNUInstallDirs)

target_include_directories(${APPNAME} INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/RingBuffer>
)

# Installation
if(RINGBUFFER_INSTALL)
    # Install the library
    install(TARGETS RingBuffer
        EXPORT RingBufferTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/RingBuffer
    )

    # Install headers
    install(FILES ringbuffer.hpp
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/RingBuffer
    )

    # Export target configuration
    install(EXPORT RingBufferTargets
        FILE RingBufferTargets.cmake
        NAMESPACE jnk0le::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/RingBuffer
    )

    include(CMakePackageConfigHelpers)

    configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/RingBufferConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/RingBuffer
    )

    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/RingBufferConfig.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/RingBuffer
    )
endif()