POT_FILE=locale/audacity_website.pot
PO_FILES_ORIG := $(wildcard locale/*.po)
PO_FILES := $(PO_FILES_ORIG:%.po=%/LC_MESSAGES/audacity_website.po)
MO_FILES := $(PO_FILES:%.po=%.mo)

all: $(MO_FILES)

clean:
	rm -f $(MO_FILES) $(PO_FILES) $(POT_FILE)
	rm -f $(PO_FILES:%=%~)

# Build the POT file (message catalog template).
SOURCES := $(wildcard *.php */*.php)
XGETTEXT=xgettext
XGETTEXT_ARGS=-L php --add-comments="i18n-hint" --from-code="UTF-8"

$(POT_FILE): $(SOURCES)
	find . -name '*.php' ! -path './old/*' \
	  | xargs $(XGETTEXT) $(XGETTEXT_ARGS) -o $@
	chmod go+r $@

# Update the PO files (translated message catalogs).
MSGMERGE=msgmerge
MSGMERGE_ARGS=-U --backup=none

%/LC_MESSAGES/audacity_website.po: %.po $(POT_FILE)
	mkdir -p $(@:%/audacity_website.po=%)
	cp -f $< $@
	$(MSGMERGE) $(MSGMERGE_ARGS) $@ $(POT_FILE)
	touch $@

# Compile the PO files into MO files.
MSGFMT=msgfmt
MSGFMT_ARGS=-fv

%.mo: %.po
	$(MSGFMT) $(MSGFMT_ARGS) -o $@ $<

.PHONY: clean all
