############################################################################
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.  The
# ASF licenses this file to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance with the
# License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.
#
############################################################################

ifneq ($(CONFIG_LVGL_UNITTESTS),)

LVGL_PATH ?= ${shell pwd}/lvgl

UNITY_CSRCS := $(shell find ${LVGL_PATH}/tests/unity/ -type f -name '*.c')
TEST_IMAGES_SRC := $(shell find ${LVGL_PATH}/tests/test_images/ -type f -name '*.c')
TEST_ASSETS_SRC := $(shell find ${LVGL_PATH}/tests/src/test_assets/ -type f -name '*.c')

CFLAGS += -DLV_TEST_OPTION=7 ${INCDIR_PREFIX}${LVGL_PATH}/tests/unity \
          ${INCDIR_PREFIX}${LVGL_PATH}/tests/src \
          -DLV_CONF_PATH=${LVGL_PATH}/tests/src/lv_test_conf.h \
          -DLV_BUILD_TEST=1 -DTEST_FONT_PATH='"/data/font/MiSansW_Regular.ttf"' \
          -DREF_IMGS_PATH='"/data/ref_imgs/"'

ifeq ($(CONFIG_LV_TEST_COVERAGE), y)
CFLAGS += --coverage
endif

CXXFLAGS += ${CFLAGS}

RESOURCE_DIR := $(APPDIR)/$(CONFIG_ARCH_BOARD_CUSTOM_DIR)../common/resource/


#    ${LVGL_PATH}/tests/src/test_cases/test_fs.c \
    ${LVGL_PATH}/tests/src/test_cases/test_anim_timeline.c \

TEST_CASE_FILES := \
    ${LVGL_PATH}/tests/src/test_cases/test_config.c \
    ${LVGL_PATH}/tests/src/test_cases/test_math.c \
    ${LVGL_PATH}/tests/src/test_cases/test_anim.c \
    ${LVGL_PATH}/tests/src/test_cases/test_margin_align.c \
    ${LVGL_PATH}/tests/src/test_cases/test_style.c \
    ${LVGL_PATH}/tests/src/test_cases/widgets/test_btnmatrix.c \
    ${LVGL_PATH}/tests/src/test_cases/widgets/test_calendar.c \
    ${LVGL_PATH}/tests/src/test_cases/widgets/test_spinbox.c \
    ${LVGL_PATH}/tests/src/test_cases/widgets/test_list.c \
    ${LVGL_PATH}/tests/src/test_cases/widgets/test_table.c \
    ${LVGL_PATH}/tests/src/test_cases/widgets/test_roller.c \
    ${LVGL_PATH}/tests/src/test_cases/widgets/test_line.c \
    ${LVGL_PATH}/tests/src/test_cases/widgets/test_scale.c \
    ${LVGL_PATH}/tests/src/test_cases/widgets/test_tabview.c \
    ${LVGL_PATH}/tests/src/test_cases/widgets/test_span.c \
    ${LVGL_PATH}/tests/src/test_cases/widgets/test_win.c \
    ${LVGL_PATH}/tests/src/test_cases/widgets/test_textarea.c \
    ${LVGL_PATH}/tests/src/test_cases/widgets/test_spinner.c \
    ${LVGL_PATH}/tests/src/test_cases/widgets/test_led.c \
    ${LVGL_PATH}/tests/src/test_cases/widgets/test_msgbox.c \
    ${LVGL_PATH}/tests/src/test_cases/widgets/test_obj_flags.c \
    ${LVGL_PATH}/tests/src/test_cases/widgets/test_label.c \
    ${LVGL_PATH}/tests/src/test_cases/widgets/test_btn.c \
    ${LVGL_PATH}/tests/src/test_cases/widgets/test_imgfont.c \
    ${LVGL_PATH}/tests/src/test_cases/widgets/test_arc.c \
    ${LVGL_PATH}/tests/src/test_cases/widgets/test_obj_tree.c \
    ${LVGL_PATH}/tests/src/test_cases/widgets/test_chart.c \
    ${LVGL_PATH}/tests/src/test_cases/widgets/test_checkbox.c \
    ${LVGL_PATH}/tests/src/test_cases/widgets/test_switch.c \
    ${LVGL_PATH}/tests/src/test_cases/widgets/test_slider.c \
    ${LVGL_PATH}/tests/src/test_cases/widgets/test_bar.c \
    ${LVGL_PATH}/tests/src/test_cases/test_grid.c \
    ${LVGL_PATH}/tests/src/test_cases/libs/test_memmove.c \
    ${LVGL_PATH}/tests/src/test_cases/libs/test_qrcode.c \
    ${LVGL_PATH}/tests/src/test_cases/libs/test_freetype.c \
    ${LVGL_PATH}/tests/src/test_cases/libs/test_barcode.c \
    ${LVGL_PATH}/tests/src/test_cases/libs/test_tiny_ttf.c \
    ${LVGL_PATH}/tests/src/test_cases/test_margin_grid.c \
    ${LVGL_PATH}/tests/src/test_cases/test_event.c \
    ${LVGL_PATH}/tests/src/test_cases/test_tree.c \
    ${LVGL_PATH}/tests/src/test_cases/test_snapshot.c \
    ${LVGL_PATH}/tests/src/test_cases/test_demo_stress.c \
    ${LVGL_PATH}/tests/src/test_cases/cache/test_cache.c \
    ${LVGL_PATH}/tests/src/test_cases/draw/test_draw_vector.c \
    ${LVGL_PATH}/tests/src/test_cases/draw/test_draw_label.c \
    ${LVGL_PATH}/tests/src/test_cases/draw/test_draw_sw_rotate.c \
    ${LVGL_PATH}/tests/src/test_cases/draw/test_image_formats.c \
    ${LVGL_PATH}/tests/src/test_cases/draw/test_clip_corner.c \
    ${LVGL_PATH}/tests/src/test_cases/draw/test_draw_blend.c \
    ${LVGL_PATH}/tests/src/test_cases/test_mem.c \
    ${LVGL_PATH}/tests/src/test_cases/test_margin_flex.c \
    ${LVGL_PATH}/tests/src/test_cases/test_screen_load.c \
    ${LVGL_PATH}/tests/src/test_cases/test_group.c \
    ${LVGL_PATH}/tests/src/test_cases/test_demo_widgets.c \
    ${LVGL_PATH}/tests/src/test_cases/test_txt.c \
    ${LVGL_PATH}/tests/src/test_cases/test_array.c

 
ifeq ($(CONFIG_LV_USE_FS_MEMFS), y)
TEST_CASE_FILES += \
    ${LVGL_PATH}/tests/src/test_cases/test_font_loader.c
endif

ifeq ($(CONFIG_LV_BUILD_EXAMPLES), y)
TEST_CASE_FILES += \
    ${LVGL_PATH}/tests/src/test_cases/widgets/test_dropdown.c
endif

ifeq ($(CONFIG_LV_USE_ANIMIMAGE), y)
TEST_CASE_FILES += \
    ${LVGL_PATH}/tests/src/test_cases/widgets/test_animimg.c
endif

ifeq ($(CONFIG_LV_USE_LIBJPEG_TURBO), y)
TEST_CASE_FILES += \
    ${LVGL_PATH}/tests/src/test_cases/libs/test_libjpeg_turbo.c
endif

ifeq ($(CONFIG_LV_USE_TJPGD), y)
TEST_CASE_FILES += \
    ${LVGL_PATH}/tests/src/test_cases/libs/test_tjpgd.c
endif

ifeq ($(CONFIG_LV_USE_LIBPNG), y)
TEST_CASE_FILES += \
    ${LVGL_PATH}/tests/src/test_cases/libs/test_libpng.c
endif

ifeq ($(CONFIG_LV_USE_LODEPNG), y)
TEST_CASE_FILES += \
    ${LVGL_PATH}/tests/src/test_cases/widgets/test_image.c \
    ${LVGL_PATH}/tests/src/test_cases/libs/test_lodepng.c
endif

ifeq ($(CONFIG_LV_USE_DEMO_RENDER), y)
TEST_CASE_FILES += \
    ${LVGL_PATH}/tests/src/test_cases/draw/test_render_to_xrgb8888.c \
    ${LVGL_PATH}/tests/src/test_cases/draw/test_render_to_argb8888.c \
    ${LVGL_PATH}/tests/src/test_cases/draw/test_render_to_rgb888.c \
    ${LVGL_PATH}/tests/src/test_cases/draw/test_render_to_rgb565.c
endif

ifeq ($(CONFIG_LV_USE_OBJ_ID), y)
TEST_CASE_FILES += \
    ${LVGL_PATH}/tests/src/test_cases/widgets/test_objid.c
endif

ifeq ($(CONFIG_LV_USE_OBJ_PROPERTY), y)
TEST_CASE_FILES += \
    ${LVGL_PATH}/tests/src/test_cases/widgets/test_obj_property.c \
    ${LVGL_PATH}/tests/src/test_cases/widgets/test_image_property.c
endif

ifeq ($(CONFIG_LV_USE_OBSERVER), y)
TEST_CASE_FILES += \
    ${LVGL_PATH}/tests/src/test_cases/test_observer.c \
    ${LVGL_PATH}/tests/src/test_cases/test_bindings.c
endif

ifeq ($(CONFIG_LV_USE_SVG), y)
TEST_CASE_FILES += \
    ${LVGL_PATH}/tests/src/test_cases/draw/test_draw_svg.c \
    ${LVGL_PATH}/tests/src/test_cases/test_svg.c
endif


$(foreach TEST_CASE_FILE, ${TEST_CASE_FILES}, \
    $(eval include ${LVGL_PATH}/tests/Makefile_Tests) \
)

$(shell touch ${LVGL_PATH}/tests/build_test_nuttx/test_entries.lock)

TEST_CASE_RUNNERS += $(shell find ${LVGL_PATH}/tests/build_test_nuttx -type f -name '*.c')

UNITY_CSRCS += \
        ${LVGL_PATH}/tests/src/lv_test_indev.c \
        ${LVGL_PATH}/tests/src/lv_test_init.c \
        ${LVGL_PATH}/tests/src/lv_test_helpers.c \
        ${TEST_ASSETS_SRC} \
        ${TEST_IMAGES_SRC}

UNITY_CSRCS += ${TEST_CASE_RUNNERS}

PROGNAME = lvgltest
MAINSRC = ${LVGL_PATH}/tests/src/lvgltest.c ${UNITY_CSRCS}
PRIORITY = $(CONFIG_LVGL_UNITTESTS_PRIORITY)
STACKSIZE = $(CONFIG_LVGL_UNITTESTS_STACKSIZE)
MODULE = ${CONFIG_LVGL_UNITTESTS}

postinstall::
    $(shell cp -r ${LVGL_PATH}/tests/ref_imgs ${RESOURCE_DIR})

distclean::
	$(call DELFILE, ${LVGL_PATH}/tests/build_test_nuttx/*.lock)
	$(call DELFILE, ${LVGL_PATH}/tests/build_test_nuttx/*.gcno)
	$(call DELFILE, ${LVGL_PATH}/tests/build_test_nuttx/*.c)
	$(call DELFILE, ${LVGL_PATH}/tests/build_test_nuttx/*.h)
	$(shell rm -rdf ${RESOURCE_DIR}/ref_imgs)

endif #CONFIG_LVGL_UNITTESTS
