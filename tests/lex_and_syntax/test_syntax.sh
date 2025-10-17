#!/bin/bash

# Define paths for the compiler and test samples
COMPILER="./build/cactc"
SAMPLES_DIR="./tests/lex_and_syntax/samples"

# Color codes for pretty printing
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Initialize counters
total_tests=0
passed_tests=0
failed_tests=0

# First, check if the compiler executable exists
if [ ! -f "$COMPILER" ]; then
    echo -e "${RED}Error: Compiler '$COMPILER' not found.${NC}"
    echo "Please run 'make' to build it first."
    exit 1
fi

echo "Running syntax tests..."
echo "========================================="

# Loop through all .cact test files
for test_file in "$SAMPLES_DIR"/*.cact; do
    # Check if any files match the pattern to avoid errors when the directory is empty
    [ -e "$test_file" ] || continue
    
    ((total_tests++))
    filename=$(basename "$test_file")

    # Run your compiler, redirecting all output to /dev/null to keep the test output clean.
    # We only care about the exit code.
    "$COMPILER" "$test_file" > /dev/null 2>&1
    exit_code=$?

    # Case 1: Filename contains "true", so we expect a zero exit code (success)
    if [[ "$filename" == *"true"* ]]; then
        if [ "$exit_code" -eq 0 ]; then
            echo -e "[${GREEN}PASSED${NC}] $filename (Expected: 0, Got: 0)"
            ((passed_tests++))
        else
            echo -e "[${RED}FAILED${NC}] $filename (Expected: 0, Got: $exit_code)"
            ((failed_tests++))
        fi
    # Case 2: Filename contains "false", so we expect a non-zero exit code (failure)
    elif [[ "$filename" == *"false"* ]]; then
        if [ "$exit_code" -ne 0 ]; then
            echo -e "[${GREEN}PASSED${NC}] $filename (Expected: non-zero, Got: $exit_code)"
            ((passed_tests++))
        else
            echo -e "[${RED}FAILED${NC}] $filename (Expected: non-zero, Got: 0)"
            ((failed_tests++))
        fi
    else
        # Warn about and skip files that don't follow the naming convention
        echo -e "[${YELLOW}SKIPPED${NC}] File '$filename' does not contain 'true' or 'false' in its name."
        ((total_tests--))
    fi
done

echo "========================================="
echo "Test Summary:"
echo "Total tests run: $total_tests"
echo -e "Passed: ${GREEN}$passed_tests${NC}"
echo -e "Failed: ${RED}$failed_tests${NC}"
echo "========================================="

# Exit with a non-zero status if any test failed (useful for CI/CD pipelines)
if [ "$failed_tests" -ne 0 ]; then
    exit 1
fi

exit 0

