# vcpkg port for depz-sensor-sdk-cpp.
#
# Sources come from the public mirror (depz-ai/depz-sensor-sdk), tag v${VERSION}
# — the same tag/tarball as the C SDK, so both ports share one SHA512.
# On release (docs/RELEASING.md): run sync-mirror for the new version first,
# then refresh SHA512 with the hash of the new mirror tarball:
#   curl -fsSL https://github.com/depz-ai/depz-sensor-sdk/archive/refs/tags/v<X.Y.Z>.tar.gz | sha512sum
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO depz-ai/depz-sensor-sdk
    REF "v${VERSION}"
    SHA512 70bf9e54e7ae48dd713061772244907ecff1854bc99b2646c6022398f607692a94884d52312a203b511f3221a20bb6ca0cb3632c7b40536903ffe3d1bb140043
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
