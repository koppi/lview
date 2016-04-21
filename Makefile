CFLAGS=`pkg-config --cflags gtk+-2.0`
LDLIBS=`pkg-config --libs gtk+-2.0` -lm

VERSION = 0.4

all: lview

clean:
	rm -rf core *~ lview

dist: clean
	cd .. && tar cvfz lview-$(VERSION).tgz lview && cd lview
