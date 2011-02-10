MAN1 =
MAN3 =
MAN7 = zfl.7 \
    zfl_base.7 \
    zfl_blob.7 \
    zfl_config.7 \
    zfl_device.7 \
    zfl_hash.7 \
    zfl_list.7 \
    zfl_msg.7 \
    zfl_rpc.7 \
    zfl_rpcd.7 \
    zfl_thread.7
MAN_DOC = $(MAN1) $(MAN3) $(MAN7)

MAN_TXT = $(MAN1:%.1=%.txt)
MAN_TXT += $(MAN3:%.3=%.txt)
MAN_TXT += $(MAN7:%.7=%.txt)
MAN_HTML = $(MAN_TXT:%.txt=%.html)

if INSTALL_MAN
dist_man_MANS = $(MAN_DOC)
endif

EXTRA_DIST = asciidoc.conf $(MAN_TXT)
if BUILD_DOC
EXTRA_DIST += $(MAN_HTML)
endif

MAINTAINERCLEANFILES = $(MAN_DOC) $(MAN_HTML)

dist-hook : $(MAN_DOC) $(MAN_HTML)

if BUILD_DOC
SUFFIXES=.html .txt .xml .1 .3 .7

.txt.html:
	asciidoc -d manpage -b xhtml11 -f asciidoc.conf \
		-azfl_version=@PACKAGE_VERSION@ $<
.txt.xml:
	asciidoc -d manpage -b docbook -f asciidoc.conf \
		-azfl_version=@PACKAGE_VERSION@ $<
.xml.1:
	xmlto man $<
.xml.3:
	xmlto man $<
.xml.7:
	xmlto man $<
endif
