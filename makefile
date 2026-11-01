get = $(shell sed -n 's/^$1=//p' PROJECT)

CC       = $(call get,cc)
CPP      = $(call get,c++)
SRCDIR   = $(call get,src)
BINDIR   = $(call get,bin)
OBJDIR   = $(call get,obj)
NAME     = $(call get,name)
VERSION  = $(call get,version)
CFLAGS   = $(call get,cc-flags)
LDFLAGS  = $(call get,ld-flags)
CPPFLAGS = $(call get,cpp-flags)
CPLUSPLUSFLAGS = $(call get,c++-flags)

BINARYDIR = $(BINDIR)/$(VERSION)
BINARY    = $(BINARYDIR)/$(NAME)

CSRC := $(shell find $(SRCDIR) -name '*.c')
CPPSRC := $(shell find $(SRCDIR) -name '*.cpp')
COBJ := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(CSRC))
CPPOBJ := $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(CPPSRC))
DEP := $(COBJ:.o=.d) $(CPPOBJ:.o=.d)

CFLAGS += -MMD -MP

.PHONY: all clean run init remove

all: $(BINARY)

$(BINARY): $(COBJ) $(CPPOBJ) | $(BINARYDIR)
	$(CPP) -o $@ $^ $(LDFLAGS) 
	cp PROJECT $(SRCDIR)/assets/
	@[ -d "$(SRCDIR)/assets" ] && cp -r $(SRCDIR)/assets/* $(BINARYDIR) || true

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -DVERSION='"$(VERSION)"' -c -o $@ $<
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	mkdir -p $(dir $@)
	$(CPP) $(CPPFLAGS) $(CPLUSPLUSFLAGS) -DVERSION="$(VERSION)" -c -o $@ $<

$(OBJDIR) $(BINARYDIR):
	mkdir -p $@

-include $(DEP)

run: $(BINARY)
	cd $(BINARYDIR) && ./$(NAME)

clean:
	rm -rf $(OBJDIR) $(BINARYDIR)

init:
	git init
	mkdir src obj bin src/assets
	echo "bin\nobj" > .gitignore
	echo "name=unknown\nversion=1.0.0\nsrc=src\nobj=obj\nbin=bin\n\ncc=cc\nc++=c++\ncc-flags=-O0\nc++-flags=\nld-flags=-lm\ncpp-flags=\n" > PROJECT
	echo "#include <stdio.h>\n\nint main(void) { printf(\"Hello, World!\x0A\"); return 0; }\n" > src/main.c
remove:
	rm -rf .git $(SRCDIR) $(BINDIR) $(OBJDIR)
	rm .gitignore
	rm PROJECT
