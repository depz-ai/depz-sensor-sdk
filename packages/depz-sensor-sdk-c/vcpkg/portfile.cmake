# vcpkg port for depz-sensor-sdk-c.
#
# Sources come from the public mirror (depz-ai/depz-sensor-sdk), tag v${VERSION}.
# On release (docs/RELEASING.md): run sync-mirror for the new version first,
# then refresh SHA512 with the hash of the new mirror tarball:
#   curl -fsSL https://github.com/depz-ai/depz-sensor-sdk/archive/refs/tags/v<X.Y.Z>.tar.gz | sha512sum
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO depz-ai/depz-sensor-sdk
    REF "v${VERSION}"
    SHA512 6aa9a92feb637e8daad83611eda487a9f0e946b5dd880d836f54b2fc1c72c0920454af3067975c244b8279effa44d701698f90a8a62a3d004130b4704e5179b0
    HEAD_REF main
)

# The C SDK lives in a subdirectory of the repository.
vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}/packages/depz-sensor-sdk-c"
    OPTIONS
        -DDEPZ_SENSOR_SDK_C_BUILD_TESTS=OFF
        -DDEPZ_SENSOR_SDK_C_BUILD_EXAMPLES=OFF
)

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(
    PACKAGE_NAME depz-sensor-sdk-c
    CONFIG_PATH lib/cmake/depz-sensor-sdk-c
)

# Static-only C library: drop the empty include dir duplication in debug.
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

vcpkg_install_copyright(FILE_LIST
    "${SOURCE_PATH}/packages/depz-sensor-sdk-c/README.md"
    # the embedded ams TMF8829 sensor firmware (MIT)
    "${SOURCE_PATH}/packages/depz-sensor-sdk-c/licenses/tmf8829/LICENSES-MIT.TXT")
