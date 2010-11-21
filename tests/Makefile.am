INCLUDES = -g -I$(top_srcdir) -I$(top_srcdir)/gedit $(GEDIT_DEBUG_FLAGS) $(GEDIT_CFLAGS)

noinst_PROGRAMS = $(TEST_PROGS) $(TEST_GTK_PROGS)
progs_ldadd     = $(top_builddir)/gedit/libgedit.la

TEST_PROGS			= document-input-stream
document_input_stream_SOURCES	= document-input-stream.c
document_input_stream_LDADD	= $(progs_ldadd)

TEST_PROGS			+= document-output-stream
document_output_stream_SOURCES	= document-output-stream.c
document_output_stream_LDADD	= $(progs_ldadd)

TEST_PROGS			+= document-loader
document_loader_SOURCES		= document-loader.c
document_loader_LDADD		= $(progs_ldadd)

TEST_PROGS			+= document-saver
document_saver_SOURCES		= document-saver.c
document_saver_LDADD		= $(progs_ldadd)

TEST_GTK_PROGS			= test-rounded-frame
test_rounded_frame_SOURCES	= test-rounded-frame.c
test_rounded_frame_LDADD	= $(progs_ldadd)

TEST_GTK_PROGS			+= test-overlay
test_overlay_SOURCES		= test-overlay.c
test_overlay_LDADD		= $(progs_ldadd)

TESTS = $(TEST_PROGS)

EXTRA_DIST = setup-document-saver.sh

-include $(top_srcdir)/git.mk
