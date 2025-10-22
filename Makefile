# ==============================================================================
# netstatAX25 Makefile
# ==============================================================================

# CORE VARIABLES
# ------------------------------------------------------------------------------
TARGET    = netstatAX25
SOURCE    = netstatAX25.c
CC        = gcc
CFLAGS    = -Wall -Wextra -O2

# Standard object file definition
OBJS      = $(SOURCE:.c=.o) 

# Installation paths (standard for /usr/local)
PREFIX    = /usr/local
BINDIR    = $(PREFIX)/sbin

# DOCUMENTATION FILES
# ------------------------------------------------------------------------------
DOC_EN_HTML = AX25_proc_docs_EN.html
DOC_FR_HTML = AX25_proc_docs_FR.html
DOC_EN_PDF  = AX25_proc_docs_EN.pdf
DOC_FR_PDF  = AX25_proc_docs_FR.pdf

# All generated documentation files
DOC_HTMLS = $(DOC_EN_HTML) $(DOC_FR_HTML)

# ==============================================================================
# COMPILATION RULES
# ==============================================================================

# Default target (run with 'make' or 'make all')
all: $(TARGET)

# Linking rule to create the final executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

# Rule to compile the source file
$(OBJS): $(SOURCE)
	$(CC) $(CFLAGS) -c $< -o $@

# ==============================================================================
# MANAGEMENT RULES
# ==============================================================================

# Installation of the executable
install: all
	@echo "Installing $(TARGET) to $(BINDIR)..."
	mkdir -p $(BINDIR)
	install -m 755 $(TARGET) $(BINDIR)
	@echo "Installation complete."

# Uninstallation of the executable
uninstall:
	@echo "Uninstalling $(TARGET) from $(BINDIR)..."
	rm -f $(BINDIR)/$(TARGET)
	@echo "Uninstallation complete."

# Cleaning temporary files (ONLY removes executable and object files)
# **Crucially, this does NOT remove any *.pdf files.**
clean:
	@echo "Cleaning binaries and object files..."
	rm -f $(TARGET) $(OBJS) *~

# Rule to generate documentation (Placeholder command)
# This target is independent and does not trigger 'clean'.
docs:
	@echo "Generating documentation files..."
	# Replace the command below with your actual documentation tool for HTML generation
	# Example: pandoc -o $(DOC_EN_HTML) doc_en.md
	@echo "NOTE: Generate $(DOC_HTMLS) and $(DOC_EN_PDF) $(DOC_FR_PDF) here."
	@echo "Output files expected: $(DOC_HTMLS) and PDFs."

# Rule to clean up all generated documentation files
docs_clean:
	@echo "Cleaning ALL generated documentation files (HTMLs and PDFs)..."
	rm -f $(DOC_HTMLS) $(DOC_EN_PDF) $(DOC_FR_PDF)

.PHONY: all clean install uninstall docs docs_clean
