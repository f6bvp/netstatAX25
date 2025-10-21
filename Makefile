# Variables
CC = gcc
# CFLAGS: Compiler flags (strict checks + optimization)
CFLAGS = -Wall -Wextra -O2 -g -std=c99
# CPPFLAGS: Preprocessor flags (adds POSIX/GNU support for strdup, strtok_r)
CPPFLAGS = -D_GNU_SOURCE

SOURCE = netstatAX25.c
TARGET = netstat_procutils

# Documentation filenames (Updated)
DOC_EN_HTML = AX25_proc_docs_EN.html
DOC_FR_HTML = AX25_proc_docs_FR.html
DOC_EN_PDF = AX25_proc_docs_EN.pdf
DOC_FR_PDF = AX25_proc_docs_FR.pdf


.PHONY: all clean docs pdf_clean

# Default rule (executed by 'make')
all: $(TARGET)

# Compilation rule
$(TARGET): $(SOURCE)
	# Compile the main utility
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $(TARGET) $(SOURCE)

## Documentation Rules

# Rule to generate PDF documentation from the final HTML files
docs: pdf_clean
	@echo "--- Generating Documentation PDFs ---"
	# Convert French HTML to PDF
	pandoc $(DOC_FR_HTML) -o $(DOC_FR_PDF) --pdf-engine=pdflatex
	# Convert English HTML to PDF
	pandoc $(DOC_EN_HTML) -o $(DOC_EN_PDF) --pdf-engine=pdflatex
	@echo "Documentation PDFs generated: $(DOC_FR_PDF) and $(DOC_EN_PDF)"

# Rule to clean up files generated during PDF creation (temporary LaTeX files)
pdf_clean:
	rm -f *.aux *.log *.toc *.out

## Cleaning Rules

# General cleaning rule: Removes the executable and generated PDFs
clean: pdf_clean
	rm -f $(TARGET) $(DOC_FR_PDF) $(DOC_EN_PDF)
