#!/bin/bash

set -e  # exits on any error

echo "==========================================="
echo "Building and Running ECS Tests"
echo "==========================================="

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # no color

print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# create build directory
print_status "Setting up build directory..."
rm -rf build
mkdir -p build
cd build

# configure project
print_status "Configuring project with CMake..."
cmake ..

# build tests
print_status "Building tests..."
make ecs_tests -j$(nproc)

# run tests with detailed output
print_status "Running tests..."
if ./ecs_tests --gtest_output=xml:test_results.xml; then
    echo "==========================================="
    print_status "✅ All tests passed!"
    echo "==========================================="
    
    # show test summary
    echo ""
    print_status "Test Summary:"
    ./ecs_tests --gtest_brief=1
    
    # TODO: come back to
    # show component growth stats specifically
    echo ""
    print_status "Component Growth/Shrinkage Results:"
    ./ecs_tests --gtest_filter="*Growth*:*Shrinkage*" --gtest_brief=1
else
    echo "==========================================="
    print_error "❌ Some tests failed!"
    echo "==========================================="
    exit 1
fi

echo ""
print_status "Test completed......"
print_status "Detailed results: $(pwd)/test_results.xml"