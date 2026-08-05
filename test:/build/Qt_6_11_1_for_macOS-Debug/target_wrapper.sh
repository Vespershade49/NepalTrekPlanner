#!/bin/bash
DYLD_FRAMEWORK_PATH=/Users/kshitijsen/Qt/6.11.1/macos/lib${DYLD_FRAMEWORK_PATH:+:$DYLD_FRAMEWORK_PATH}
export DYLD_FRAMEWORK_PATH
QT_PLUGIN_PATH=/Users/kshitijsen/Qt/6.11.1/macos/plugins${QT_PLUGIN_PATH:+:$QT_PLUGIN_PATH}
export QT_PLUGIN_PATH
exec "$@"
