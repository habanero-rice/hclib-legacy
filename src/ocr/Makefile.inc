RUNTIME_OCR_SRC=ocr

noinst_LTLIBRARIES += libruntime_ocr.la
libhclib_la_LIBADD += libruntime_ocr.la

libruntime_ocr_la_SOURCES = \
$(RUNTIME_OCR_SRC)/runtime-ocr.c

libruntime_ocr_la_CFLAGS = $(CFLAGS) $(INCLUDES_DIR) -I$(OCR_ROOT)/include
libruntime_ocr_la_LDFLAGS = $(OCR_LDFLAGS) $(OCR_LIBS)