export ROOT_DIR=$(shell git rev-parse --show-toplevel)
DIR=$(shell source $(ROOT_DIR)/env.sh &&  echo $$EXT_DIR )/pteditor
all:
	$(CC) $(CFLAGS) -c -fPIC $(DIR)/ptedit.c -o ptedit.o
	$(CC) $(CFLAGS) -shared ptedit.o -o libptedit.so
