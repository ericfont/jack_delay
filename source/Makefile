# -----------------------------------------------------------------------------
#
#  Copyright (C) 2012-2018 Fons Adriaensen <fons@linuxaudio.org>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# -----------------------------------------------------------------------------


PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin

VERSION = 0.4.2
CPPFLAGS += -MMD -MP -DVERSION=\"$(VERSION)\"
CXXFLAGS += -O2 -Wall
CXXFLAGS += -march=native
LDLIBS += -ljack -lasound -lrt


all:	jack_delay


JACK_DELAY_O =	jack_delay.o mtdm.o
jack_delay:	$(JACK_DELAY_O)
	$(CXX) $(LDFLAGS) -o $@ $(JACK_DELAY_O) $(LDLIBS)


install:	jack_delay
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 jack_delay $(DESTDIR)$(BINDIR)


uninstall:
	rm -f $(DESTDIR)$(BINDIR)/jack_delay


clean:
	/bin/rm -f *~ *.o *.a *.d *.so jack_delay

