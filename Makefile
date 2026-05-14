# ch32-workbench top-level Makefile
#
# Iterates apps/*/ and delegates to each app's own Makefile. Each app
# declares its own DEFAULT_BOARD; passing BOARD=<name> at this level
# overrides that default for every app in this invocation.

APPS          := $(notdir $(patsubst %/,%,$(wildcard apps/*/)))
BOARD_HEADERS := $(filter-out boards/board.h,$(wildcard boards/*.h))
BOARDS        := $(notdir $(BOARD_HEADERS:.h=))

# Propagate BOARD only if the user set it explicitly at this level.
ifdef BOARD
  MAKE_BOARD_ARG := BOARD=$(BOARD)
endif

.PHONY: all clean list-apps list-boards help

all:
	@for d in $(APPS); do \
		$(MAKE) -C apps/$$d $(MAKE_BOARD_ARG) || exit $$?; \
	done

clean:
	@for d in $(APPS); do \
		$(MAKE) -C apps/$$d clean || exit $$?; \
	done

list-apps:
	@printf '%s\n' $(APPS)

list-boards:
	@printf '%s\n' $(BOARDS)

help:
	@echo "ch32-workbench — top-level targets:"
	@echo "  make                   build all apps for their default boards"
	@echo "  make BOARD=<name>      build all apps for the named board"
	@echo "  make clean             remove all build artifacts"
	@echo "  make list-apps         list discovered apps"
	@echo "  make list-boards       list discovered board headers"
	@echo ""
	@echo "Per-app:"
	@echo "  cd apps/<name> && make [BOARD=<name>]"
	@echo "  cd apps/<name> && make flash"
