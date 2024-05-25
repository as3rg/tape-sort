ifdef VCPKG
TOOLCHAIN = -DCMAKE_TOOLCHAIN_FILE=$(VCPKG)/vcpkg/scripts/buildsystems/vcpkg.cmake
endif

build-tests:
	cmake . -B=build -DCMAKE_BUILD_TYPE=Release -DTESTS=ON $(TOOLCHAIN)
	cmake --build build

tests: build-tests
	./build/tests/tape-tests

build:
	cmake . -B=build -DCMAKE_BUILD_TYPE=Release;
	cmake --build build

run: build
	./build/util/tape-util $(args)

clean:
	rm -rf cmake-build-*
	rm -rf build
	rm -rf tmp
