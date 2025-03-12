#!/bin/bash

CUR_DIR=$(cd $(dirname $0); pwd)
cd ${CUR_DIR}

# run LVGL code formatter
${CUR_DIR}/scripts/code-format.py

# set the option to fail a pipeline if any command within it fails
set -o pipefail

if ! (git diff --exit-code --color=always | tee /tmp/lvgl_diff.patch); then
    echo "Please apply the preceding diff to your code or run scripts/code-format.py"
    exit 1
fi

echo "LVGL code format check passed"
exit 0
