###############################################################################
#
# Installation DIRECTORIES
#
# Change these if you want to install somewhere else.

###############################################################################
#
# GENERAL
#
PROJECT_DIR := $(shell pwd)
#PROM    = alsa_ladspa

INCLUDES	=	-I.
LIBRARIES	=	-ldl -lm
CFLAGS		=	$(INCLUDES) -Wall  #-O3
LOCAL_LDFLAGS   =      -L./$(LADSPA_PATH) -lRK_AudioProcess
CXXFLAGS	=	$(CFLAGS)
PLUGINS		:=	./$(LADSPA_PATH)/rk_DCBDoingc.so ./$(LADSPA_PATH)/Equalizer.so

###############################################################################
#
# RULES TO BUILD PLUGINS FROM C OR C++ CODE
#

./$(LADSPA_PATH)/%.so:	%.c ladspa.h
	$(CC) $(CFLAGS) -o ./$*.o -c -fPIC ./$*.c
	$(LD) -o ./$(LADSPA_PATH)/$*.so ./$*.o -shared $(LOCAL_LDFLAGS)


###############################################################################
#
# TARGETS
#

install:	targets

targets:	$(PLUGINS) 


###############################################################################
#
# UTILITIES
#

always:	

clean:
	-rm -f `find . -name "*.o"` ./*.so
	-rm -f `find .. -name "*~"`
	-rm -f *.bak core score.srt
	-rm -f *.bb *.bbg *.da *-ann gmon.out bb.out
	-rm -f `find .. -name "*.class"`

backup:		clean
	(cd ../../;							\
	tar czf `date '+../backup/ladspa_sdk.%Y%m%d%H%M.tgz'` ladspa_sdk/)

###############################################################################
