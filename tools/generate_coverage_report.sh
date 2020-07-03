#!/bin/sh

git clone https://github.com/easimer-net/ld46 ld46

pushd ld46

git submodule init
git submodule update

mkdir -p build
cd build

cmake \
    -DBOX2D_BUILD_DOCS=OFF \
    -DBOX2D_BUILD_TESTBED=OFF \
    -DBOX2D_BUILD_UNIT_TESTS=ON \
    -DCMAKE_CXX_FLAGS="-maes -msse4 -mfma --coverage" \
    ..

make -j$(nproc) entity_gen_tests
ctest

rm -rf ../../report/
mkdir -p ../../report/
gcovr -r .. -f ../entity_gen/ --html --html-details -o ../../report/report.html

popd
rm -rf ld46
