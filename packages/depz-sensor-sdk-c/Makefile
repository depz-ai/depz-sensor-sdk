# depz-sensor-sdk-c — convenience targets (the canonical build is CMake).

.PHONY: build test docs clean

build:           ## configure + build the static library and the vector runner
	cmake -B build -S .
	cmake --build build

test: build      ## run the golden-vector / replay suite (13 tests)
	ctest --test-dir build --output-on-failure

docs:            ## regenerate docs/api.md + per-sensor api.md from the header
	python3 scripts/gen_api_md.py

clean:
	rm -rf build
