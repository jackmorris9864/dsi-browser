
# Nintendo DS / DSi browser build

TARGET      := dsi-browser
BUILD       := build
SOURCES     := source
INCLUDES    :=

ARCH        := -mthumb -mthumb-interwork

CFLAGS      := -g -Wall -O2 \
               -mcpu=arm946e-s \
               -mtune=arm946e-s \
               $(ARCH) \
               -DARM9

CXXFLAGS    := $(CFLAGS) -fno-rtti -fno-exceptions

ASFLAGS     := -g $(ARCH)
LDFLAGS     := -specs=ds_arm9.specs

LIBS        := -lnds9 -ldswifi9
LIBDIRS     :=

include $(DEVKITARM)/ds_rules
