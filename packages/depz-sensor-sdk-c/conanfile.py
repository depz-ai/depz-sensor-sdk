from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout
from conan.tools.files import copy
import os


class DepzSensorSdkCConan(ConanFile):
    name = "depz-sensor-sdk-c"
    version = "0.1.0"
    license = "MIT"
    author = "DEPZ AI"
    url = "https://github.com/depz-ai/depz-sensor-sdk-native"
    description = (
        "Contract-first C11 decode/codec SDK for the DEPZ USB sensor line "
        "(SR04, VL53L8CX/CH, BNO086): CRCs, packet framing, USB identity, "
        "firmware container parsing, and the per-sensor decode layer."
    )
    topics = ("sensors", "usb", "codec", "depz", "embedded")

    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}

    # Ship the sources this recipe builds from. Paths are relative to this
    # conanfile (packages/depz-sensor-sdk-c/).
    exports_sources = (
        "CMakeLists.txt",
        "cmake/*",
        "include/*",
        "src/*",
        "tests/*",
        "README.md",
    )

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")
        # Pure C library: it has no C++ standard library nor compiler-cpp ABI.
        self.settings.rm_safe("compiler.cppstd")
        self.settings.rm_safe("compiler.libcxx")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = CMakeToolchain(self)
        # Consumers must never build our monorepo-vector test suite.
        tc.cache_variables["DEPZ_SENSOR_SDK_C_BUILD_TESTS"] = False
        tc.generate()
        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
        copy(self, "README.md", self.source_folder,
             os.path.join(self.package_folder, "licenses"))

    def package_info(self):
        self.cpp_info.libs = ["depz_sensor_sdk"]
        if self.settings.os in ("Linux", "FreeBSD"):
            self.cpp_info.system_libs = ["m"]
        # Match the installed CMake package so find_package() consumers and
        # Conan CMakeDeps consumers see the same namespaced target.
        self.cpp_info.set_property("cmake_file_name", "depz-sensor-sdk-c")
        self.cpp_info.set_property("cmake_target_name", "depz::sensor_sdk_c")
