#!/bin/bash

CUR_DIR=$(cd $(dirname $0); pwd)
cd ${CUR_DIR}

# run LVGL code formatter
${CUR_DIR}/scripts/code-format.py

# generate lv_conf_internal.h
${CUR_DIR}/scripts/lv_conf_internal_gen.py

# set the option to fail a pipeline if any command within it fails
set -o pipefail

if ! (git diff --exit-code --color=always | tee /tmp/lvgl_diff.patch); then
    echo "Please apply the preceding diff to your code."
    echo "Or run scripts/code-format.py and scripts/lv_conf_internal_gen.py to update your code."
    exit 1
fi

echo "LVGL code format check passed"
exit 0
