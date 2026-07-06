# vcpkg port for depz-sensor-sdk-cpp.
#
# The SDK lives in a subdirectory of the monorepo, so SOURCE_SUBDIR points
# vcpkg at packages/depz-sensor-sdk-cpp. On release, publish a git tag
# `cpp-v0.1.0` and replace SHA512 below with the value vcpkg prints on the
# first (deliberately-failing) build, or from:
#   vcpkg_from_github(... REF cpp-v0.1.0 SHA512 0 ...)  # then read the error
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO depz-ai/depz-sensor-sdk-and-viewer
    REF "cpp-v${VERSION}"
    # TODO(release): replace with the real archive SHA512 for tag cpp-v0.1.0.
    SHA512 0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
    HEAD_REF main
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}/packages/depz-sensor-sdk-cpp"
    OPTIONS
        -DDEPZ_SENSOR_SDK_CPP_BUILD_TESTS=OFF
)

vcpkg_cmake_install()

# Move the CMake config package into vcpkg's expected share/<port> location.
vcpkg_cmake_config_fixup(
    PACKAGE_NAME depz-sensor-sdk-cpp
    CONFIG_PATH lib/cmake/depz-sensor-sdk-cpp
)

# Static-only library: no headers or CMake files belong in the debug tree.
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/packages/depz-sensor-sdk-cpp/LICENSE")
