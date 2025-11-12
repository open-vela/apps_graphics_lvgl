#!/bin/bash

start_time=$(date +%s)

CUR_DIR=$(cd $(dirname $0); pwd)
cd ${CUR_DIR}/..
echo "Current directory: $CUR_DIR"

echo "Checking prerequisites..."

# List of packages to install
apt_packages=$(sed -n '/sudo apt install/,/[^\\]$/p' scripts/install-prerequisites.sh | \
    sed 's/sudo apt install //' | \
    sed 's/\\//g' | \
    tr -s ' ' | \
    sed 's/ $//')
echo "Required packages: $apt_packages"

missing_packages_count=0

# Check if each package is installed
for package in $apt_packages; do
    if ! dpkg -l | grep -q "^ii  $package"; then
        echo "Package $package is not installed."
        missing_packages_count=$((missing_packages_count + 1))
    else
        echo "Package $package is already installed."
    fi
done

# List of Python packages to install
pip_packages=$(grep 'pip3 install' scripts/install-prerequisites.sh | sed 's/pip3 install //')
echo "Required Python packages: $pip_packages"

# Check if each Python package is installed
for package in $pip_packages; do
    if ! pip3 show $package > /dev/null 2>&1; then
        echo "Python package $package is not installed"
        missing_packages_count=$((missing_packages_count + 1))
    else
        echo "Python package $package is already installed."
    fi
done

if [ $missing_packages_count -gt 0 ]; then
    echo "Missing $missing_packages_count packages detected. Please run 'scripts/install-prerequisites.sh' to install them."
    exit 1
else
    echo "All required packages are installed."
fi

# Check gcovr version
gcovr_version=$(gcovr --version | grep -oP '\d+\.\d+')
required_version=8.2

if (( $(echo "$gcovr_version < $required_version" | bc -l) )); then
    echo "gcovr version is $gcovr_version, which is lower than $required_version"
    echo "Please run 'pip install --upgrade gcovr' to install a newer version."
    exit 1
else
    echo "gcovr version is $gcovr_version, which meets the requirement."
fi

# Get versions of g++, gcc, and gcov
gpp_version=$(g++ -dumpversion)
gcc_version=$(gcc -dumpversion)
gcov_version=$(gcov -dumpversion | grep -oP '\d+\.\d+.\d+' | head -n 1 | cut -d'.' -f1) # only major version

# Check if g++, gcc, and gcov versions are the same
if [ "$gpp_version" != "$gcc_version" ] || [ "$gcc_version" != "$gcov_version" ]; then
    echo "Versions mismatch detected:"
    echo "g++ version: $gpp_version"
    echo "gcc version: $gcc_version"
    echo "gcov version: $gcov_version"
    echo "g++, gcc, and gcov must have the same version."
    exit 1
else
    echo "g++, gcc, and gcov versions match: $gpp_version"
fi

# Run tests for 32-bit build and test
echo "Running tests for 32-bit build and test..."

export NON_AMD64_BUILD=1
./tests/main.py --clean --report build test
if [ $? -eq 0 ]; then
    echo "32-bit tests passed."
    rm -rf ./tests/report-32bit
    mv ./tests/report ./tests/report-32bit
else
    echo "32-bit tests failed!"
    exit 1
fi

# Run tests for 64-bit build and test
echo "Running tests for 64-bit build and test..."

unset NON_AMD64_BUILD
./tests/main.py --clean --report build test
if [ $? -eq 0 ]; then
    echo "64-bit tests passed."
    rm -rf ./tests/report-64bit
    mv ./tests/report ./tests/report-64bit
else
    echo "64-bit tests failed!"
    exit 1
fi

echo "Checking gcov coverage..."
./scripts/check_gcov_coverage.py
if [ $? -ne 0 ]; then
    echo "Gcov coverage check failed!"
    exit 1
fi

new_png_files=$(git status --porcelain | grep "\?\? .*\.png")
if [ -n "$new_png_files" ]; then
    echo "New untracked PNG files detected:"
    echo "$new_png_files"
    echo "Please add them to the repository and commit them."
    exit 1
else
    echo "No new untracked PNG files detected."
fi

elapsed_time=$(($(date +%s) - start_time))
minutes=$((elapsed_time / 60))
seconds=$((elapsed_time % 60))
echo "Tests completed in $minutes minutes and $seconds seconds."

echo "See gcover reports in:"
echo "  $PWD/tests/report-32bit/index.html"
echo "  $PWD/tests/report-64bit/index.html"

echo "LVGL all tests passed."
exit 0
