BIN=klogforward  klogcollect
default: $(BIN)

install: default
	cp $(BIN) $(DESTDIR)/bin/

clean:
	-rm $(BIN)
