#
# Makefile fragment for the library
#

LIB_CFLAGS := $(CFLAGS)

LIB_SRCFILES := 

LIB_SRCFILES += \
  lib/assert/__panic.c

LIB_SRCFILES += \
  lib/ctype/__ctype.c \
	lib/ctype/__tolower.c \
	lib/ctype/__toupper.c \
  lib/ctype/isalnum.c \
	lib/ctype/isalpha.c \
	lib/ctype/iscntrl.c \
	lib/ctype/isdigit.c \
	lib/ctype/isgraph.c \
	lib/ctype/islower.c \
	lib/ctype/isprint.c \
	lib/ctype/ispunct.c \
	lib/ctype/isspace.c \
	lib/ctype/isupper.c \
	lib/ctype/isxdigit.c \
	lib/ctype/tolower.c \
	lib/ctype/toupper.c

LIB_SRCFILES += \
  lib/dirent/getdents.c

LIB_SRCFILES += \
  lib/errno/errno.c

LIB_SRCFILES += \
	lib/fcntl/open.c

LIB_SRCFILES += \
	lib/float/__fvalues.c

LIB_SRCFILES += \
	lib/setjmp/longjmp.S \
	lib/setjmp/setjmp.S

LIB_SRCFILES += \
  lib/stdio/__printf.c \
	lib/stdio/perror.c \
	lib/stdio/printf.c \
	lib/stdio/putchar.c \
	lib/stdio/puts.c \
	lib/stdio/snprintf.c \
	lib/stdio/vprintf.c \
	lib/stdio/vsnprintf.c

LIB_SRCFILES += \
	lib/stdlib/abort.c \
	lib/stdlib/abs.c \
	lib/stdlib/atoi.c \
	lib/stdlib/atol.c \
	lib/stdlib/bsearch.c \
	lib/stdlib/div.c \
	lib/stdlib/exit.c \
	lib/stdlib/getenv.c \
	lib/stdlib/labs.c \
	lib/stdlib/ldiv.c \
	lib/stdlib/rand.c \
	lib/stdlib/srand.c \
	lib/stdlib/strtol.c \
	lib/stdlib/strtoul.c

LIB_SRCFILES += \
	lib/string/memchr.c \
	lib/string/memcmp.c \
	lib/string/memcpy.c \
	lib/string/memmove.c \
	lib/string/memset.c \
	lib/string/strcat.c \
	lib/string/strchr.c \
	lib/string/strcmp.c \
	lib/string/strcpy.c \
	lib/string/strcspn.c \
	lib/string/strerror.c \
	lib/string/strlen.c \
	lib/string/strncat.c \
	lib/string/strncmp.c \
	lib/string/strncpy.c \
	lib/string/strpbrk.c \
	lib/string/strrchr.c \
	lib/string/strspn.c \
	lib/string/strstr.c \
	lib/string/strtok.c

LIB_SRCFILES += \
	lib/sys/stat/fstat.c \
	lib/sys/stat/mkdir.c \
	lib/sys/stat/stat.c

LIB_SRCFILES += \
  lib/sys/wait/wait.c \
	lib/sys/wait/waitpid.c

LIB_SRCFILES += \
	lib/time/asctime.c \
	lib/time/gmtime.c \
	lib/time/mktime.c \
	lib/time/strftime.c \
	lib/time/time.c

LIB_SRCFILES += \
  lib/unistd/chdir.c \
	lib/unistd/close.c \
	lib/unistd/execl.c \
	lib/unistd/execlp.c \
	lib/unistd/execv.c \
	lib/unistd/execvp.c \
  lib/unistd/fork.c \
	lib/unistd/getpid.c \
	lib/unistd/getppid.c \
	lib/unistd/read.c \
	lib/unistd/sbrk.c \
	lib/unistd/write.c

LIB_OBJFILES := $(patsubst %.c, $(OBJ)/%.o, $(LIB_SRCFILES))
LIB_OBJFILES := $(patsubst %.S, $(OBJ)/%.o, $(LIB_OBJFILES))

CRT_SRCFILES := \
	lib/crt0.S \
	lib/crti.S \
	lib/crtn.S

CRT_OBJFILES := $(patsubst %.S, $(OBJ)/%.o, $(CRT_SRCFILES))

$(OBJ)/lib/%.o: lib/%.c $(OBJ)/.vars.LIB_CFLAGS
	@echo "+ CC  $<"
	@mkdir -p $(@D)
	$(V)$(CC) $(LIB_CFLAGS) -c -o $@ $<

$(OBJ)/lib/%.o: lib/%.S $(OBJ)/.vars.LIB_CFLAGS
	@echo "+ AS  $<"
	@mkdir -p $(@D)
	$(V)$(CC) $(LIB_CFLAGS) -c -o $@ $<

$(OBJ)/lib/libc.a: $(LIB_OBJFILES)
	@echo "+ AR  $@"
	$(V)$(AR) r $@ $(LIB_OBJFILES)

lib: $(OBJ)/lib/libc.a $(CRT_OBJFILES)
