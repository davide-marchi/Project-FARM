# Compilatore da usare
CC			=  gcc
# Aggiungo alcuni flags di compilazione
CFLAGS	    += -std=c99 -Wall -pedantic -g -D_DEFAULT_SOURCE
# Directory includes
INCL_DIR	= ./includes
# Flag inclusione 
INCLUDES	= -I ${INCL_DIR}
# Libreria per compilazione
LIBS        = -pthread

# OBJECTS DEPENDENCIES
FARM_OBJS =  objs/MasterWorker.o objs/Master.o objs/Worker.o objs/Collector.o objs/threadpool.o objs/list.o
GENERAFILE_OBJ = objs/generafile.o

# BIN
FARM_BIN = farm
GENERAFILE_BIN = generafile

# TARGETS
TARGETS		= $(FARM_BIN) \
			  $(GENERAFILE_BIN) \

# PHONY
.PHONY: all test cleantest cleanall
.SUFFIXES: .c .h

# Regola di default
all		: $(TARGETS)

# Eseguibile farm
${FARM_BIN}: $(FARM_OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) -o $@ $(FARM_OBJS) $(LIBS)

# Eseguibile generafile
${GENERAFILE_BIN}: $(GENERAFILE_OBJ) 
	$(CC) $(CFLAGS) -o $@ $(GENERAFILE_OBJ)

objs/generafile.o: src/test/generafile.c
	$(CC) $(CFLAGS) $< -c -fPIC -o $@

objs/%.o: src/strutture/%.c $(INCL_DIR)/%.h
	$(CC) $(CFLAGS) $(INCLUDES) $< -c -fPIC -o $@

objs/%.o: src/masterworker/%.c $(INCL_DIR)/%.h
	$(CC) $(CFLAGS) $(INCLUDES) $< -c -fPIC -o $@

objs/%.o: src/collector/%.c $(INCL_DIR)/%.h
	$(CC) $(CFLAGS) $(INCLUDES) $< -c -fPIC -o $@

# Esecuzione dello script per testare
test:
		bash test.sh

# Per rimuovere i file generati dal test
cleantest:
		rm -f -r file*
		rm -f -r testdir
		rm -f -r expected.txt

# Per rimuovere tutti gli eseguibili e gli oggetti
cleanall:
		rm -f -r farm
		rm -f -r objs/*.o
		rm -f -r collector
		rm -f -r generafile
		rm -f -r file*
		rm -f -r testdir
		rm -f -r expected.txt