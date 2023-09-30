# Show commands that make uses
VERBOSE = FALSE

# Directory layout.
PROJDIR := $(realpath $(CURDIR)/)
SOURCEDIR := $(PROJDIR)/src
TESTDIR := $(PROJDIR)/tests

RELEASEDIR := $(PROJDIR)/release
DEBUGDIR := $(PROJDIR)/debug

CFLAGS = -Wall -Werror -std=c11 -I$(SOURCEDIR) -I$(TESTDIR)
LDLIBS = -lm
ifeq ($(DEBUG),1)
	CFLAGS += -g3
	LDLIBS +=
	BUILDDIR := $(DEBUGDIR)
else
	CFLAGS += -fPIC -O3 -DNDEBUG
	LDLIBS += -fPIC
	BUILDDIR := $(RELEASEDIR)
endif

# Target executable
TEST  = test
TEST_TARGET = $(BUILDDIR)/$(TEST)

# Compiler and compiler options
CC = clang

# Add this list to the VPATH, the place make will look for the source files
VPATH = $(TESTDIR):$(SOURCEDIR)

# Create a list of *.c files in DIRS
SOURCES = $(wildcard $(TESTDIR)/*.c) $(wildcard $(SOURCEDIR)/*.c)
HEADERS = $(wildcard $(TESTDIR)/*.h) $(wildcard $(SOURCEDIR)/*.h)

# Define object files for all sources, and dependencies for all objects
OBJS:=$(subst $(SOURCEDIR), $(BUILDDIR), $(SOURCES:.c=.o))
OBJS:=$(subst $(TESTDIR), $(BUILDDIR), $(OBJS))

DEPS := $(OBJS:.o=.d)

# Print a bunch of info to help debugging make.
ifeq ($(VERBOSE),TRUE)
    $(info ----------------- variables ----------------- )
    $(info                                               )
    $(info $$(CC) $(CC)                                  )
    $(info                                               )
    $(info $$(TEST_TARGET) $(TEST_TARGET)                )
    $(info                                               )
    $(info $$(CFLAGS) $(CFLAGS)                          )
    $(info                                               )
    $(info $$(LDLIBS) $(LDLIBS)                          )
    $(info                                               )
    $(info $$(VPATH)                                     )
    $(foreach DIR, $(subst :, , $(VPATH)), $(info $(DIR)))
    $(info                                               )
    $(info $$(SOURCES)                                   )
    $(foreach SRC, $(SOURCES), $(info $(SRC))            )
    $(info                                               )
    $(info $$(HEADERS)                                   )
    $(foreach SRC, $(HEADERS), $(info $(SRC))            )
    $(info                                               )
    $(info $$(OBJS)                                      )
    $(foreach OBJ, $(OBJS), $(info $(OBJ))               )
    $(info                                               )
    $(info $$(DEPS)                                      )
    $(foreach DEP, $(DEPS), $(info $(DEP))               )
    $(info                                               )
    $(info --------------------------------------------- )
endif

# Hide or not the calls depending on VERBOSE
ifeq ($(VERBOSE),TRUE)
	HIDE = 
else
	HIDE = @
endif

.PHONY: all clean directories test

all: makefile directories $(TEST_TARGET)

$(TEST_TARGET): directories makefile  $(OBJS)
	@echo
	@echo Linking $@
	$(HIDE)$(CC) $(OBJS) $(LDLIBS) -o $@

-include $(DEPS)

# Generate rules
$(BUILDDIR)/%.o: %.c makefile
	@echo
	@echo Building $@
	$(HIDE)$(CC) -c $(CFLAGS) -o $@ $< -MMD

directories:
	@echo
	@echo "Creating directory $(BUILDDIR)"
	$(HIDE)mkdir -p $(BUILDDIR) 2>/dev/null

test: directories makefile $(TEST_OBJS) $(OBJS)
	@echo
	@echo Linking $@
	$(HIDE)$(CC)  $(TEST_OBJS) $(OBJS) $(LDLIBS) -o $(TEST_TARGET)
	$(HIDE) $(TEST_TARGET)

clean:
	-$(HIDE)rm -rf $(DEBUGDIR) $(RELEASEDIR) 2>/dev/null
	@echo
	@echo Cleaning done!

