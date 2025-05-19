#!/bin/bash

CUR_DIR=$(cd $(dirname $0); pwd)
cd ${CUR_DIR}

# run LVGL code formatter
${CUR_DIR}/scripts/code-format.py

# generate lv_conf_internal.h
${CUR_DIR}/scripts/lv_conf_internal_gen.py

# generate style API
${CUR_DIR}/scripts/style_api_gen.py

# set the option to fail a pipeline if any command within it fails
set -o pipefail

if ! (git diff --exit-code --color=always | tee /tmp/lvgl_diff.patch); then
    echo "Please apply the preceding diff to your code."
    echo "Or run the following commands to update your code:"
    echo "    scripts/code-format.py             # Update code formatting"
    echo "    scripts/lv_conf_internal_gen.py    # Generate lv_conf_internal.h"
    echo "    scripts/style_api_gen.py           # Generate style API"
    exit 1
fi

echo "LVGL code format check passed"
exit 0
