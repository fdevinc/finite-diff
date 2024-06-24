# Detects whether this is a top-level project
get_directory_property(HAS_PARENT PARENT_DIRECTORY)
if(HAS_PARENT)
    set(FINITE_DIFF_TOPLEVEL_PROJECT OFF)
else()
    set(FINITE_DIFF_TOPLEVEL_PROJECT ON)
endif()

# Check required CMake version
set(REQUIRED_CMAKE_VERSION "3.14.0")
if(FINITE_DIFF_TOPLEVEL_PROJECT)
    cmake_minimum_required(VERSION ${REQUIRED_CMAKE_VERSION})
else()
    # Don't use cmake_minimum_required here to avoid implicitly overriding parent policies
    if(${CMAKE_VERSION} VERSION_LESS ${REQUIRED_CMAKE_VERSION})
        message(FATAL_ERROR "CMake required version to build finite-diff is ${REQUIRED_CMAKE_VERSION}")
    endif()
endif()

################################################################################

project(FiniteDiff
        DESCRIPTION "A simple finite difference library."
        LANGUAGES CXX
        VERSION 1.0.2)

option(FINITE_DIFF_INSTALL "Install CMake targets during install step."
       ${FINITE_DIFF_TOPLEVEL_PROJECT})

# Set default minimum C++ standard
if(FINITE_DIFF_TOPLEVEL_PROJECT)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/finitediff/")
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/recipes/")

################################################################################
# Finite Diff Library
################################################################################

add_library(finitediff
    src/finitediff.cpp
)
add_library(finitediff::finitediff ALIAS finitediff)

# Public include directory
target_include_directories(
  finitediff
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:include>)

################################################################################
# Optional Definitions
################################################################################

# For MSVC, do not use the min and max macros.
target_compile_definitions(finitediff PUBLIC NOMINMAX)

################################################################################
# Dependencies
################################################################################

# Eigen
find_package(Eigen3 REQUIRED NO_MODULE)
target_link_libraries(finitediff PUBLIC Eigen3::Eigen)

# Logger
find_package(spdlog REQUIRED)
target_link_libraries(finitediff PUBLIC spdlog::spdlog)

################################################################################
# Compiler options
################################################################################

# Use C++11
target_compile_features(finitediff PUBLIC cxx_std_20)

################################################################################
# Installation
################################################################################
if (FINITE_DIFF_INSTALL)
    include(GNUInstallDirs)

    # Install library headers.
    install(
        DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    # Install library binaries.
    install(
        TARGETS finitediff
        EXPORT ${PROJECT_NAME}Targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    include(CMakePackageConfigHelpers)

    configure_package_config_file(
        FiniteDiffConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/finitediff)

    write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    VERSION ${PACKAGE_VERSION}
    COMPATIBILITY SameMajorVersion)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
                ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/finitediff)

    # Perform installation.
    install(EXPORT ${PROJECT_NAME}Targets
            FILE ${PROJECT_NAME}Targets.cmake
            NAMESPACE finitediff::
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/finitediff
    )
endif()
