# libvuknob
Base library for vuKNOB - the Digital Audio Workstation for Android.

# license
libvuknob is released under the GNU Lesser General Public License, version 2.

Please note that libvuknob depends on additional third party libraries that has
their own licenses. If you distribute software including or using libvuknob make
sure you follow gpl v2, and also the additional licensing requirements specified in
the third party libraries.

# requirements

 * A development host running some form of GNU/Linux (tested on Ubuntu 15.04)
 * the Android SDK
 * the Android NDK
 * The SAMSUNG Professional Audio libraries
 * a local libsvgandroid repository ( git clone https://github.com/pltxtra/libsvgandroid.git )
   - make sure you configure it and build it properly

# configure

Quick:

```
./configure --ndk-path ~/Source/Android/android-ndk --sdk-path ~/Source/Android/android-sdk-linux --libsvgandroid-path ~/Source/Android/libsvgandroid --libkamoflage-path ~/Source/Android/libkamoflage --target-platform android-10 --bootstrap
```

--bootstrap will download required third party libraries and cross compile them for Android. (Except Samsung Professional Audio)


# SAMSUNG Professional Audio

Samsung Professional Audio - SAPA - is required to build libvuknob. You need to acquire it manually
from Samsung's developer web page. Make sure you install these files in the prereqs directory:

```
lib/libjack.so
lib/libsapaclient.a
include/IAPAInterfaceV2.h
include/IJackClientInterfaceV2.h
include/APACommon.h
include/IAPAResponseInterface.h
include/jack
include/jack/thread.h
include/jack/types.h
include/jack/transport.h
include/jack/midiport.h
include/jack/net.h
include/jack/weakjack.h
include/jack/systemdeps.h
include/jack/jack.h
include/jack/ringbuffer.h
include/jack/session.h
include/jack/statistics.h
include/jack/intclient.h
include/jack/jslist.h
include/jack/weakmacros.h
include/jack/control.h
include/IJackClientInterface.h
include/IAPAInterface.h
```

You also need the following JARs in build/libs/:

```
android-support-v4.jar
professionalaudio-v2.1.0.jar
sdk-v1.0.0.jar
```

# compiling

either:

```
make debug
```

or:

```
make release
```

# usage

Include libvuknob.so as a prebuilt shared library in your Android.mk file. Please refer to the
Android NDK documentation for information about specifics.

Include libvuknob.jar in your libs directory. Please refer to the Android SDK documentation for
information about specifics.
