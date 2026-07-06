# NOTE (fill at release):
#   REF must point at the C-SDK release tag `v0.1.1`.
#   SHA512 is the placeholder "0"; replace it with the real hash, e.g.
#     vcpkg install depz-sensor-sdk-c   # first run prints the actual SHA512
#   or `vcpkg x-add-version` / the CI helper. Do NOT ship with SHA512 "0".
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO depz-ai/depz-sensor-sdk
    REF v0.1.2
    SHA512 a55df8ece752f5288df0fc1c5db09b5caccf4a300e37adbe29a40434252fec7cfea901eb78236f878e838e10b30c1cf766ca1187c0afea574bcb356c998b06d8
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
