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
    SHA512 8b2c497bedc251679d9dc372b1f00077bd5c92646f653f87d414a3316ca312bfc029c949658d50234621bd5da8991150b184de214dc50682f21f2688b839ecdc
    HEAD_REF main
)

# The C SDK lives in a subdirectory of the monorepo.
vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}/packages/depz-sensor-sdk-c"
    OPTIONS
        -DDEPZ_SENSOR_SDK_C_BUILD_TESTS=OFF
)

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(
    PACKAGE_NAME depz-sensor-sdk-c
    CONFIG_PATH lib/cmake/depz-sensor-sdk-c
)

# Static-only C library: drop the empty include dir duplication in debug.
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/packages/depz-sensor-sdk-c/README.md")
