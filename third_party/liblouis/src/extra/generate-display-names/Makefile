CC := gcc
DISPLAY_NAMES_FILE := display-names

export LOUIS_TABLEPATH = $(CURDIR)/../../tables
export GOPATH = $(CURDIR)/.go-packages

.PHONY : check
check : generate
	./$< $(DISPLAY_NAMES_FILE) >$<.log 2>$<.log

generate : generate.o displayLanguage.a ../../liblouis/liblouis.la
	../../libtool --tag=CC --mode=link $(CC) -o $@ $^ -lpthread

generate.o : generate.c displayLanguage.h
	$(CC) -I. -I../../liblouis -g -O2 -c -o $@ $<

displayLanguage.a displayLanguage.h : displayLanguage.go
	go get golang.org/x/text/language/display
	go build -buildmode=c-archive $<

.PHONY : clean
clean :
	rm -rf displayLanguage.a displayLanguage.h $(GOPATH)
