VERSION_NAME := $(shell git tag | grep -Po "v(\d+\.)+\d+" | sort -V | head -n1)

lib_LTLIBRARIES = libvuknobclient.la

# libvuknobclient
libvuknobclient_la_CPPFLAGS = \
 -std=c++11 \
 $(libkamoflage_CFLAGS) $(gnuVG_CFLAGS) $(libkamoflage_CFLAGS)/kamoflage \
 -Wall -Werror -Wfatal-errors \
 -D__RI__CLIENT_SIDE \
 -DCONFIG_DIR='"${prefix}/share/vuknob"' \
 -DLIBSVG_EXPAT \
 -D__SATAN_USES_FLOATS \
 -DVERSION_NAME='"$(VERSION_NAME)"'

libvuknobclient_la_LDFLAGS =  -L${prefix}/lib $(libkamoflage_LIBS)

libvuknobclient_la_SOURCES = \
generic_platform_interface.cc \
generic_platform_interface.hh \
serialize.cc serialize.hh \
remote_interface.cc remote_interface.hh \
engine_code/global_control_object.cc engine_code/global_control_object.hh \
engine_code/handle_list.cc engine_code/handle_list.hh \
engine_code/sequence.cc engine_code/sequence.hh \
engine_code/base_machine.cc engine_code/base_machine.hh \
engine_code/machine_api.cc engine_code/machine_api.hh \
engine_code/client.cc engine_code/client.hh \
engine_code/scales_control.cc engine_code/scales_control.hh \
ui_code/fling_animation.cc ui_code/fling_animation.hh \
ui_code/tap_detector.cc ui_code/tap_detector.hh \
ui_code/svg_loader.cc ui_code/svg_loader.hh \
ui_code/sequencer.cc ui_code/sequencer.hh \
ui_code/ui_stack.cc ui_code/ui_stack.hh \
ui_code/gnuvg_corner_button.cc ui_code/gnuvg_corner_button.hh \
ui_code/logo_screen.cc ui_code/logo_screen.hh \
ui_code/gnuvg_scaleeditor.cc ui_code/gnuvg_scaleeditor.hh \
ui_code/gnuvg_livepad.cc ui_code/gnuvg_livepad.hh \
ui_code/gnuvg_listview.cc ui_code/gnuvg_listview.hh \
ui_code/gnuvg_machine_creator.cc ui_code/gnuvg_machine_creator.hh \
ui_code/gnuvg_connector.cc ui_code/gnuvg_connector.hh \
ui_code/gnuvg_connection_list.cc ui_code/gnuvg_connection_list.hh \
ui_code/pattern_editor.cc ui_code/pattern_editor.hh \
ui_code/scale_slide.cc ui_code/scale_slide.hh \
ui_code/knob_instance.cc ui_code/knob_instance.hh \
ui_code/knob_editor.cc ui_code/knob_editor.hh \
ui_code/timelines.cc ui_code/timelines.hh \
ui_code/popup_window.cc ui_code/popup_window.hh \
ui_code/popup_menu.cc ui_code/popup_menu.hh \
ui_code/main_menu.cc ui_code/main_menu.hh \
ui_code/settings_screen.cc ui_code/settings_screen.hh \
ui_code/loop_settings.cc ui_code/loop_settings.hh
