#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
output="${TMPDIR:-/tmp}/open_ride_dash_task_frequency_test"

g++ \
    -std=c++17 \
    -Wall \
    -Wextra \
    -Werror \
    -I"${repo_root}/test/fakes" \
    -I"${repo_root}/src" \
    "${repo_root}/test/task_frequency/task_frequency_test.cpp" \
    -o "${output}"

"${output}"
