PY = python -m compileall -f

CONFIG = config

SRC = src
SRC_CONF = $(SRC)/$(CONFIG)

TARGETS = ipcam_hf
TARGET_CONF = $(TARGETS)/$(CONFIG)

all: clean $(TARGETS)

$(TARGETS):
	$(PY) $(SRC)
	mkdir -p $(TARGET_CONF)
	mv $(SRC)/*.pyc $(TARGETS)
	mv $(SRC_CONF)/*.pyc $(TARGET_CONF)
	rm $(TARGET_CONF)/setting.pyc
	cp $(SRC_CONF)/setting.py $(TARGET_CONF)

clean: 
	rm -rf $(TARGETS)
