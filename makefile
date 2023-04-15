# Directory layout.
PROJDIR := $(realpath $(CURDIR)/)
SOURCEDIR := $(PROJDIR)/src
TESTDIR := $(PROJDIR)/tests
DOCDIR := $(PROJDIR)/doc

RELEASEDIR := $(PROJDIR)/release
DEBUGDIR := $(PROJDIR)/debug

CFLAGS = -Wall -Werror -std=c11 -I$(SOURCEDIR)
LDLIBS = -lm
ifeq ($(DEBUG),1)
	CFLAGS += -g -DELK_PANIC_CRASH -DELK_MEMORY_DEBUG
	LDLIBS +=
	BUILDDIR := $(DEBUGDIR)
else
	CFLAGS += -fPIC -O3 -DNDEBUG
	LDLIBS += -fPIC
	BUILDDIR := $(RELEASEDIR)
endif

# Target executable
TEST  = test
TEST_TARGET = $(TESTDIR)/$(TEST)

# Compiler and compiler options
CC = clang

# Show commands that make uses
VERBOSE = TRUE

# Add this list to the VPATH, the place make will look for the source files
VPATH = $(SOURCEDIR):$(TESTDIR)

# Create a list of *.c files in DIRS
SOURCES = $(wildcard $(SOURCEDIR)/*.c)
TEST_SOURCES = $(wildcard $(TESTDIR)/*.c)
HEADERS = $(wildcard $(SOURCEDIR)/*.h)
TEST_HEADERS = $(wildcard $(TESTDIR)/*.h)

# Define object files for all sources, and dependencies for all objects
OBJS := $(subst $(SOURCEDIR), $(BUILDDIR), $(SOURCES:.c=.o))
TEST_OBJS := $(TEST_SOURCES:.c=.o)
DEPS = $(OBJS:.o=.d)
TEST_DEPS = $(TEST_OBJS:.o=.d)

# Hide or not the calls depending on VERBOSE
ifeq ($(VERBOSE),TRUE)
	HIDE = 
else
	HIDE = @
endif

.PHONY: all clean directories test doc

all: makefile directories $(TEST_TARGET)

$(TEST_TARGET): directories makefile $(TEST_OBJS) $(OBJS)
	@echo Linking $@
	$(HIDE)$(CC) $(TEST_OBJS) $(OBJS) $(LDLIBS) -o $@

-include $(DEPS)
-include $(TEST_DEPS)

# Generate rules
$(BUILDDIR)/%.o: $(SOURCEDIR)/%.c makefile
	@echo Building $@
	$(HIDE)$(CC) -c $(CFLAGS) -o $@ $< -MMD

$(TESTDIR)/%.o: $(TESTDIR)/%.c makefile
	@echo Building $@
	$(HIDE)$(CC) -c $(CFLAGS) -o $@ $< -MMD

directories:
	@echo Creating directory $<
	$(HIDE)mkdir -p $(BUILDDIR) 2>/dev/null

test: directories makefile $(TEST_OBJS) $(OBJS)
	@echo Linking $@
	$(HIDE)$(CC)  $(TEST_OBJS) $(OBJS) $(LDLIBS) -o $(TEST_TARGET)
	$(HIDE) $(TEST_TARGET)

doc: Doxyfile makefile $(SOURCES) $(HEADERS)
	$(HIDE) doxygen 2>/dev/null

clean:
	-$(HIDE)rm -rf $(DEBUGDIR) $(RELEASEDIR) $(DOCDIR) $(TESTDIR)/*.o $(TESTDIR)/*.d $(TEST_TARGET) 2>/dev/null
	@echo Cleaning done!

