#ifndef HANDMADE_H
#define HANDMADE_H

#include <assert.h>
#include <stdio.h>
#include <math.h>

#include "base.h"
#include "os.h"
#include "arena.h"
#include "wav.h"

#if defined(_WIN32)
#  include <windows.h>
#  include <dsound.h>
#  include <xinput.h>
#elif defined(__linux__)
#  include <sys/mman.h>
#  include <sys/syscall.h>
#  include <unistd.h>
#  include <time.h>
#  include <x86intrin.h>

#  include <X11/Xlib.h>
#  include <X11/Xutil.h>
#  include <X11/keysym.h>

#  include <alsa/asoundlib.h>
#else
#  warning "Unknown platform!"
#endif

#endif // HANDMADE_H
