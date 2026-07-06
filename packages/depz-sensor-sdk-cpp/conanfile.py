from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout
from conan.tools.files import copy
import os


class DepzSensorSdkCppConan(ConanFile):
    name = "depz-sensor-sdk-cpp"
    version = "0.1.0"
    license = "MIT"
    author = "DEPZ"
    url = "https://github.com/depz-ai/depz-sensor-sdk-native"
    homepage = "https://github.com/depz-ai/depz-sensor-sdk-native"
    description = (
        "C++17 decode-layer SDK for the DEPZ USB sensor line: framed-protocol "
        "parser and per-sensor wire codecs (SR04, VL53L8CX/CH, BNO086). "
        "Pure codecs, no I/O."
    )
    topics = ("sensors", "usb", "decode", "protocol", "vl53l8", "bno086", "sr04")

    settings = "os", "arch", "compiler", "build_type"
    package_type = "static-library"

    # No shared option: the SDK builds as a static library only.
    exports_sources = (
        "CMakeLists.txt",
        "cmake/*",
        "include/*",
        "src/*",
        "tests/*",
        "scripts/*",
        "LICENSE",
        "README.md",
    )

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        # Consumers never build the test suite / golden-vector harness.
        tc.variables["DEPZ_SENSOR_SDK_CPP_BUILD_TESTS"] = "OFF"
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
        copy(self, "LICENSE", src=self.source_folder,
             dst=os.path.join(self.package_folder, "licenses"))

    def package_info(self):
        self.cpp_info.libs = ["depz_sensor_sdk"]

        # Match the CMake config package so find_package(depz-sensor-sdk-cpp)
        # via Conan's CMakeDeps resolves the same target as an installed build.
        self.cpp_info.set_property("cmake_file_name", "depz-sensor-sdk-cpp")
        self.cpp_info.set_property("cmake_target_name", "depz::sensor_sdk_cpp")
        self.cpp_info.set_property(
            "cmake_target_aliases", ["depz::sensor_sdk"])
