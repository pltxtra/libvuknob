#
# VuKNOB - Based on SATAN
# Copyright (C) 2013 by Anton Persson
#
# SATAN, Signal Applications To Any Network
# Copyright (C) 2003 by Anton Persson & Johan Thim
# Copyright (C) 2005 by Anton Persson
# Copyright (C) 2006 by Anton Persson & Ted Björling
# Copyright (C) 2011 by Anton Persson
#
# About SATAN:
#   Originally developed as a small subproject in
#   a course about applied signal processing.
# Original Developers:
#   Anton Persson
#   Johan Thim
#
# http://www.733kru.org/
#
# This program is free software; you can redistribute it and/or modify it under the terms of
# the GNU General Public License version 2; see COPYING for the complete License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
# without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with this program;
# if not, write to the
# Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
#

LOCAL_PATH := $(call my-dir)
LOCAL_PATH_RESTORE := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_CPP_EXTENSION := .cc
LOCAL_MODULE    := pathvariable
LOCAL_SRC_FILES := pathvariable_template.cc
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := libvorbis
LOCAL_SRC_FILES := ../prereqs/lib/libvorbis.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := libogg
LOCAL_SRC_FILES := ../prereqs/lib/libogg.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := libvorbisenc
LOCAL_SRC_FILES := ../prereqs/lib/libvorbisenc.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libsvgandroid
LOCAL_SRC_FILES := ../libsvgandroid/export/armeabi/libsvgandroid.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libkamoflage
LOCAL_SRC_FILES := ../libkamoflage/export/armeabi/libkamoflage.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libkissfft

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
	LOCAL_CFLAGS += -mfloat-abi=softfp -mfpu=neon
else
	LOCAL_CFLAGS += -DFIXED_POINT=32
endif # TARGET_ARCH_ABI == armeabi-v7a

LOCAL_SRC_FILES =\
kiss_fft.c kiss_fftr.c
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cc

LOCAL_MODULE    := vuknob

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
	LOCAL_CFLAGS += -D__SATAN_USES_FLOATS -mfloat-abi=softfp -mfpu=neon
else
	LOCAL_CFLAGS += -D__SATAN_USES_FXP -DFIXED_POINT=32
endif # TARGET_ARCH_ABI == armeabi-v7a

LOCAL_CFLAGS += -DLIBSVG_EXPAT -DCONFIG_DIR=\"/\" -I../../asio/include -DHAVE_CONFIG_H \
-Ijni/libkamoflage/ \
-I../libsvgandroid/src_jni/ \
-I../libsvgandroid/src_jni/libsvg \
-I../libsvgandroid/prereqs/include \
-I../libkamoflage/android/src_jni/libkamoflage/ \
-I../libkamoflage/android/src_jni/ \
-I../prereqs/include/ \
-I../build/ \
-Wall \
#-D__DO_TIME_MEASURE

LOCAL_CPPFLAGS += -DASIO_STANDALONE -std=c++11

LOCAL_SRC_FILES := \
satan.cc \
android_java_interface.cc android_java_interface.hh \
static_signal_preview.cc static_signal_preview.hh \
wavloader.cc wavloader.hh \
signal.cc signal.hh \
machine.cc machine.hh machine_project_entry.cc \
dynamic_machine.cc dynamic_machine.hh \
general_tools.cc \
machine_sequencer.cc machine_sequencer.hh \
midi_export.cc midi_export.hh \
vuknob_android_audio.cc vuknob_android_audio.hh \
fixedpointmathlib.cc \
satan_project_entry.cc satan_project_entry.hh \
graph_project_entry.cc graph_project_entry.hh \
vorbis_encoder.cc vorbis_encoder.hh \
whistle_analyzer.cc \
async_operations.cc \
remote_interface.cc remote_interface.hh \
scales.cc scales.hh \
serialize.cc serialize.hh \
time_measure.cc \
\
ui_code/information_catcher.cc ui_code/information_catcher.hh \
ui_code/license_view.cc \
ui_code/vorbis_export_ui.cc \
ui_code/whistle_ui.cc \
ui_code/svg_loader.cc ui_code/svg_loader.hh \
ui_code/tracker.cc ui_code/tracker.hh \
ui_code/scale_slider.cc ui_code/scale_slider.hh \
ui_code/listview.cc ui_code/listview.hh \
ui_code/connector.cc ui_code/connector.hh \
ui_code/share_ui.cc ui_code/share_ui.hh \
ui_code/samples_editor_ng.cc \
ui_code/machine_selector_ui.cc \
ui_code/load_ui.cc ui_code/save_ui.cc \
ui_code/new_project_ui.cc \
ui_code/project_info_entry.cc \
ui_code/controller_handler.cc \
ui_code/controller_envelope.cc ui_code/controller_envelope.hh \
ui_code/canvas_widget.cc ui_code/canvas_widget.hh \
ui_code/logo_screen.cc ui_code/logo_screen.hh \
ui_code/numeric_keyboard.cc ui_code/numeric_keyboard.hh \
ui_code/top_menu.cc ui_code/top_menu.hh \
ui_code/livepad2.cc ui_code/livepad2.hh \
ui_code/pncsequencer.cc ui_code/pncsequencer.hh \
ui_code/sequencer.cc ui_code/sequencer.hh \
ui_code/timelines.cc ui_code/timelines.hh \
ui_code/midi_export_gui.cc \
ui_code/file_request_ui.cc \
ui_code/advanced_file_request_ui.cc \
ui_code/fling_animation.cc ui_code/fling_animation.hh \
ui_code/corner_button.cc ui_code/corner_button.hh \
ui_code/connection_list.cc ui_code/connection_list.hh \
ui_code/scale_editor.cc ui_code/scale_editor.hh

LOCAL_STATIC_LIBRARIES := cpufeatures libvorbis libogg libvorbisenc libkissfft
LOCAL_LDLIBS += -ldl -llog
LOCAL_SHARED_LIBRARIES := libsvgandroid libkamoflage libpathvariable

include $(BUILD_SHARED_LIBRARY)

include $(LOCAL_PATH)/dynlib/Android.mk
LOCAL_PATH := $(LOCAL_PATH_RESTORE)

include $(CLEAR_VARS)
LOCAL_MODULE    := libsapaclient
LOCAL_SRC_FILES := ../prereqs/lib/libsapaclient.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libjack
LOCAL_SRC_FILES := ../prereqs/lib/libjack.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := wave
LOCAL_C_INCLUDES := ../prereqs/include
LOCAL_SRC_FILES := samsung_pa.cc samsung_jack.cc
#LOCAL_CPP_FEATURES := exceptions
LOCAL_MODULE_TAGS := eng optional
LOCAL_LDLIBS := -llog
LOCAL_SHARED_LIBRARIES := libjack libvuknob
LOCAL_STATIC_LIBRARIES := libsapaclient
#LOCAL_ARM_MODE := arm
#LOCAL_CFLAGS := -g
include $(BUILD_SHARED_LIBRARY)

$(call import-module,cpufeatures)
