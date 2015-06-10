if CRT_ENABLED

RUNTIME_CRT_SRC=crt

noinst_LTLIBRARIES += libruntime_crt.la
libhclib_la_LIBADD += libruntime_crt.la

RT_CRT_LDFLAGS=$(CRT_LDFLAGS) $(CRT_LIBS) -lpthread
RT_CRT_CFLAGS=-I$(CRT_INSTALL)/include

libruntime_crt_la_SOURCES =              \
$(RUNTIME_CRT_SRC)/deque.c               \
$(RUNTIME_CRT_SRC)/runtime-support-crt.c \
$(RUNTIME_CRT_SRC)/runtime-crt.c

libruntime_crt_la_CFLAGS = $(CFLAGS) $(INCLUDES_DIR) $(RT_CRT_CFLAGS)
libruntime_crt_la_LDFLAGS = $(RT_CRT_LDFLAGS)

endif
