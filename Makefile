CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -Iinclude
LDFLAGS = -lsqlite3 -lpthread

SRCDIR = src
OBJDIR = obj
BINDIR = bin
TARGET = $(BINDIR)/g00j_file_share

SOURCES = $(wildcard $(SRCDIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

.PHONY: all clean install dirs

all: dirs $(TARGET)

dirs:
	@mkdir -p $(OBJDIR) $(BINDIR)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR) $(BINDIR)

install: all
	sudo cp $(TARGET) /usr/local/bin/
	sudo mkdir -p /etc/g00j-share
	sudo cp -r static /etc/g00j-share/
	sudo cp -r shared /etc/g00j-share/

run: all
	cd $(BINDIR) && ./g00j_file_share

.PHONY: all clean install dirs run 