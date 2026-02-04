# ##############################################################################
# Licensed to the Apache Software Foundation (ASF) under one or more contributor
# license agreements.  See the NOTICE file distributed with this work for
# additional information regarding copyright ownership.  The ASF licenses this
# file to you under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License.  You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations under
# the License.
#
# ##############################################################################

if(CONFIG_LVGL_UNITTESTS)
  set(LVGL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../..)

  # Find all Unity source files
  file(GLOB UNITY_CSRCS ${LVGL_PATH}/tests/unity/*.c)

  # Find all test images source files
  file(GLOB_RECURSE TEST_IMAGES_SRC ${LVGL_PATH}/tests/test_images/*.c)

  # Find all test assets source files
  file(GLOB_RECURSE TEST_ASSETS_SRC ${LVGL_PATH}/tests/src/test_assets/*.c)

  # Add test cases files
  set(TEST_CASE_FILES
      ${LVGL_PATH}/tests/src/test_cases/test_config.c
      ${LVGL_PATH}/tests/src/test_cases/test_math.c
      ${LVGL_PATH}/tests/src/test_cases/test_anim.c
      ${LVGL_PATH}/tests/src/test_cases/test_anim_precise_cb.c
      ${LVGL_PATH}/tests/src/test_cases/test_margin_align.c
      ${LVGL_PATH}/tests/src/test_cases/test_style.c
      ${LVGL_PATH}/tests/src/test_cases/widgets/test_btnmatrix.c
      ${LVGL_PATH}/tests/src/test_cases/widgets/test_calendar.c
      ${LVGL_PATH}/tests/src/test_cases/widgets/test_spinbox.c
      ${LVGL_PATH}/tests/src/test_cases/widgets/test_list.c
      ${LVGL_PATH}/tests/src/test_cases/widgets/test_table.c
      ${LVGL_PATH}/tests/src/test_cases/widgets/test_roller.c
      ${LVGL_PATH}/tests/src/test_cases/widgets/test_line.c
      ${LVGL_PATH}/tests/src/test_cases/widgets/test_scale.c
      ${LVGL_PATH}/tests/src/test_cases/widgets/test_tabview.c
      ${LVGL_PATH}/tests/src/test_cases/widgets/test_span.c
      ${LVGL_PATH}/tests/src/test_cases/widgets/test_win.c
      ${LVGL_PATH}/tests/src/test_cases/widgets/test_textarea.c
      ${LVGL_PATH}/tests/src/test_cases/widgets/test_spinner.c
      ${LVGL_PATH}/tests/src/test_cases/widgets/test_led.c
      ${LVGL_PATH}/tests/src/test_cases/widgets/test_msgbox.c
      ${LVGL_PATH}/tests/src/test_cases/widgets/test_obj_flags.c
      ${LVGL_PATH}/tests/src/test_cases/widgets/test_label.c
      ${LVGL_PATH}/tests/src/test_cases/widgets/test_btn.c
      ${LVGL_PATH}/tests/src/test_cases/widgets/test_imgfont.c
      ${LVGL_PATH}/tests/src/test_cases/widgets/test_arc.c
      ${LVGL_PATH}/tests/src/test_cases/widgets/test_obj_tree.c
      ${LVGL_PATH}/tests/src/test_cases/widgets/test_chart.c
      ${LVGL_PATH}/tests/src/test_cases/widgets/test_checkbox.c
      ${LVGL_PATH}/tests/src/test_cases/widgets/test_switch.c
      ${LVGL_PATH}/tests/src/test_cases/widgets/test_slider.c
      ${LVGL_PATH}/tests/src/test_cases/widgets/test_bar.c
      ${LVGL_PATH}/tests/src/test_cases/test_grid.c
      ${LVGL_PATH}/tests/src/test_cases/libs/test_memmove.c
      ${LVGL_PATH}/tests/src/test_cases/libs/test_qrcode.c
      ${LVGL_PATH}/tests/src/test_cases/libs/test_freetype.c
      ${LVGL_PATH}/tests/src/test_cases/libs/test_barcode.c
      ${LVGL_PATH}/tests/src/test_cases/libs/test_tiny_ttf.c
      ${LVGL_PATH}/tests/src/test_cases/test_margin_grid.c
      ${LVGL_PATH}/tests/src/test_cases/test_event.c
      ${LVGL_PATH}/tests/src/test_cases/test_tree.c
      ${LVGL_PATH}/tests/src/test_cases/test_snapshot.c
      ${LVGL_PATH}/tests/src/test_cases/test_demo_stress.c
      ${LVGL_PATH}/tests/src/test_cases/cache/test_cache.c
      ${LVGL_PATH}/tests/src/test_cases/draw/test_draw_vector.c
      ${LVGL_PATH}/tests/src/test_cases/draw/test_draw_vector_detail.c
      ${LVGL_PATH}/tests/src/test_cases/draw/test_draw_label.c
      ${LVGL_PATH}/tests/src/test_cases/draw/test_draw_sw_rotate.c
      ${LVGL_PATH}/tests/src/test_cases/draw/test_image_formats.c
      ${LVGL_PATH}/tests/src/test_cases/draw/test_clip_corner.c
      ${LVGL_PATH}/tests/src/test_cases/draw/test_draw_blend.c
      ${LVGL_PATH}/tests/src/test_cases/test_mem.c
      ${LVGL_PATH}/tests/src/test_cases/test_margin_flex.c
      ${LVGL_PATH}/tests/src/test_cases/test_screen_load.c
      ${LVGL_PATH}/tests/src/test_cases/test_group.c
      ${LVGL_PATH}/tests/src/test_cases/test_demo_widgets.c
      ${LVGL_PATH}/tests/src/test_cases/test_txt.c
      ${LVGL_PATH}/tests/src/test_cases/test_array.c
      ${LVGL_PATH}/tests/src/test_cases/test_linear_allocator.c)

  # Add conditional test cases files based on configuration options
  if(CONFIG_LV_USE_FS_MEMFS)
    list(APPEND TEST_CASE_FILES
         ${LVGL_PATH}/tests/src/test_cases/test_font_loader.c)
  endif()

  if(CONFIG_LV_BUILD_EXAMPLES)
    list(APPEND TEST_CASE_FILES
         ${LVGL_PATH}/tests/src/test_cases/widgets/test_dropdown.c)
  endif()

  if(CONFIG_LV_USE_ANIMIMAGE)
    list(APPEND TEST_CASE_FILES
         ${LVGL_PATH}/tests/src/test_cases/widgets/test_animimg.c)
  endif()

  if(CONFIG_LV_USE_LIBJPEG_TURBO)
    list(APPEND TEST_CASE_FILES
         ${LVGL_PATH}/tests/src/test_cases/libs/test_libjpeg_turbo.c)
  endif()

  if(CONFIG_LV_USE_TJPGD)
    list(APPEND TEST_CASE_FILES
         ${LVGL_PATH}/tests/src/test_cases/libs/test_tjpgd.c)
  endif()

  if(CONFIG_LV_USE_LIBPNG)
    list(APPEND TEST_CASE_FILES
         ${LVGL_PATH}/tests/src/test_cases/libs/test_libpng.c)
  endif()

  if(CONFIG_LV_USE_LODEPNG)
    list(APPEND TEST_CASE_FILES
         ${LVGL_PATH}/tests/src/test_cases/widgets/test_image.c
         ${LVGL_PATH}/tests/src/test_cases/libs/test_lodepng.c)
  endif()

  if(CONFIG_LV_USE_DEMO_RENDER)
    list(
      APPEND
      TEST_CASE_FILES
      ${LVGL_PATH}/tests/src/test_cases/draw/test_render_to_xrgb8888.c
      ${LVGL_PATH}/tests/src/test_cases/draw/test_render_to_argb8888.c
      ${LVGL_PATH}/tests/src/test_cases/draw/test_render_to_rgb888.c
      ${LVGL_PATH}/tests/src/test_cases/draw/test_render_to_rgb565.c)
  endif()

  if(CONFIG_LV_USE_OBJ_ID)
    list(APPEND TEST_CASE_FILES
         ${LVGL_PATH}/tests/src/test_cases/widgets/test_objid.c)
  endif()

  if(CONFIG_LV_USE_OBJ_PROPERTY)
    list(APPEND TEST_CASE_FILES
         ${LVGL_PATH}/tests/src/test_cases/widgets/test_obj_property.c
         ${LVGL_PATH}/tests/src/test_cases/widgets/test_image_property.c)
  endif()

  if(CONFIG_LV_USE_OBSERVER)
    list(APPEND TEST_CASE_FILES
         ${LVGL_PATH}/tests/src/test_cases/test_observer.c
         ${LVGL_PATH}/tests/src/test_cases/test_bindings.c)
  endif()

  if(CONFIG_LV_USE_SVG)
    list(
      APPEND
      TEST_CASE_FILES
      ${LVGL_PATH}/tests/src/test_cases/draw/test_draw_svg.c
      ${LVGL_PATH}/tests/src/test_cases/test_svg.c
      ${LVGL_PATH}/tests/src/test_cases/libs/test_svg_decoder.c)
  endif()

  if(CONFIG_LV_USE_PERF_MONITOR)
    list(APPEND TEST_CASE_FILES
         ${LVGL_PATH}/tests/src/test_cases/test_perf_monitor.c)
  endif()

  # Include each test case file
  foreach(TEST_CASE_FILE ${TEST_CASE_FILES})
    include(${LVGL_PATH}/tests/cmake_nuttx/Makefile_Tests)
  endforeach()

  # Add test case runners
  file(GLOB TEST_CASE_RUNNERS
       ${CMAKE_CURRENT_BINARY_DIR}/../build_test_nuttx/*.c)

  # Add test case runners to Unity sources
  list(APPEND UNITY_CSRCS ${TEST_CASE_RUNNERS})

  # Add test images and assets source files to Unity sources
  list(
    APPEND
    UNITY_CSRCS
    ${LVGL_PATH}/tests/src/lv_test_indev.c
    ${LVGL_PATH}/tests/src/lv_test_fs.c
    ${LVGL_PATH}/tests/src/lv_test_init.c
    ${LVGL_PATH}/tests/src/lv_test_helpers.c
    ${TEST_IMAGES_SRC}
    ${TEST_ASSETS_SRC})

  # Create lvgltest executable
  nuttx_add_application(
    NAME
    lvgltest
    PRIORITY
    ${CONFIG_LVGL_UNITTESTS_PRIORITY}
    STACKSIZE
    ${CONFIG_LVGL_UNITTESTS_STACKSIZE}
    SRCS
    ${LVGL_PATH}/tests/src/lvgltest.c
    ${UNITY_CSRCS}
    COMPILE_FLAGS
    ${CMAKE_C_FLAGS}
    -DLV_TEST_OPTION=7
    -DLV_BUILD_TEST=1
    -DTEST_FONT_PATH="/data/lvgl_assets/src/test_files/fonts/noto/NotoSansSC-Regular.ttf"
    -DREF_IMGS_PATH="/data/ref_imgs/"
    -DTEST_CUSTOM_FONT_PATH="data/lvgl_assets/"
    -DREF_IMG_TOLERANCE=2   # Tolerance for different freetype version
    -DNON_AMD64_BUILD=1     # Use *.lp32.png references images
    # Remap Unity test framework symbols to avoid conflicts
    # between lvgl's bundled Unity and app-level Unity usage.
    -DUnity=lv_Unity
    -DUnityStrFail=lv_UnityStrFail
    -DUnityPrint=lv_UnityPrint
    -DUnityPrintLen=lv_UnityPrintLen
    -DUnityPrintNumberByStyle=lv_UnityPrintNumberByStyle
    -DUnityPrintNumber=lv_UnityPrintNumber
    -DUnityPrintNumberUnsigned=lv_UnityPrintNumberUnsigned
    -DUnityPrintNumberHex=lv_UnityPrintNumberHex
    -DUnityPrintMask=lv_UnityPrintMask
    -DUnityPrintFloat=lv_UnityPrintFloat
    -DUnityConcludeTest=lv_UnityConcludeTest
    -DUnityStrPass=lv_UnityStrPass
    -DUnityAssertBits=lv_UnityAssertBits
    -DUnityAssertEqualNumber=lv_UnityAssertEqualNumber
    -DUnityAssertGreaterOrLessOrEqualNumber=lv_UnityAssertGreaterOrLessOrEqualNumber
    -DUnityAssertEqualIntArray=lv_UnityAssertEqualIntArray
    -DUnityAssertEqualFloatArray=lv_UnityAssertEqualFloatArray
    -DUnityAssertFloatsWithin=lv_UnityAssertFloatsWithin
    -DUnityAssertFloatSpecial=lv_UnityAssertFloatSpecial
    -DUnityAssertNumbersWithin=lv_UnityAssertNumbersWithin
    -DUnityAssertNumbersArrayWithin=lv_UnityAssertNumbersArrayWithin
    -DUnityAssertEqualString=lv_UnityAssertEqualString
    -DUnityAssertEqualStringLen=lv_UnityAssertEqualStringLen
    -DUnityAssertEqualStringArray=lv_UnityAssertEqualStringArray
    -DUnityAssertEqualMemory=lv_UnityAssertEqualMemory
    -DUnityNumToPtr=lv_UnityNumToPtr
    -DUnityFloatToPtr=lv_UnityFloatToPtr
    -DUnityFail=lv_UnityFail
    -DUnityIgnore=lv_UnityIgnore
    -DUnityStrIgnore=lv_UnityStrIgnore
    -DUnityMessage=lv_UnityMessage
    -DUnityDefaultTestRun=lv_UnityDefaultTestRun
    -DUnitySetTestFile=lv_UnitySetTestFile
    -DUnityBegin=lv_UnityBegin
    -DUnityEnd=lv_UnityEnd
    -DUnityStrOk=lv_UnityStrOk
    -DUnityStrErr64=lv_UnityStrErr64
    -DUnityStrErrDouble=lv_UnityStrErrDouble
    -DUnityStrErrFloat=lv_UnityStrErrFloat
    -DUnityStrErrShorthand=lv_UnityStrErrShorthand
    INCLUDE_DIRECTORIES
    "${LVGL_PATH}/tests/unity"
    "${LVGL_PATH}/tests"
    "${LVGL_PATH}/tests/src"
    "${CMAKE_CURRENT_BINARY_DIR}"
    MODULE
    ${CONFIG_LVGL_UNITTESTS}
    DEPENDS
    liblvgl)

  # Copy reference images to resource directory
  set(RESOURCE_DIR
      ${CMAKE_SOURCE_DIR}/${CONFIG_ARCH_BOARD_CUSTOM_DIR}../common/resource/)
  file(COPY ${LVGL_PATH}/tests/ref_imgs DESTINATION ${RESOURCE_DIR}/)
  file(MAKE_DIRECTORY ${RESOURCE_DIR}/lvgl_assets ${RESOURCE_DIR}/lvgl_assets/src)
  file(COPY ${LVGL_PATH}/tests/test_images DESTINATION ${RESOURCE_DIR}/lvgl_assets/)
  file(COPY ${LVGL_PATH}/tests/src/test_assets DESTINATION ${RESOURCE_DIR}/lvgl_assets/src/)
  file(COPY ${LVGL_PATH}/tests/src/test_files DESTINATION ${RESOURCE_DIR}/lvgl_assets/src/)

  # Create lock file
  file(TOUCH ${CMAKE_CURRENT_BINARY_DIR}/../build_test_nuttx/test_entries.lock)
endif()
