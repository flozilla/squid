#include "squid.h"

static void fdUpdateBiggest _PARAMS((int fd, unsigned int status));

static void
fdUpdateBiggest(int fd, unsigned int status)
{
    if (fd < Biggest_FD)
	return;
    if (fd >= Squid_MaxFD) {
	debug_trap("Running out of file descriptors.\n");
	return;
    }
    if (fd > Biggest_FD) {
	if (status == FD_OPEN)
	    Biggest_FD = fd;
	else
	    debug_trap("Biggest_FD inconsistency");
	return;
    }
    /* if we are here, then fd == Biggest_FD */
    if (status != FD_CLOSE) {
	debug_trap("re-opening Biggest_FD?");
	return;
    }
    while (fd_table[Biggest_FD].open != FD_OPEN)
	Biggest_FD--;
}

void
fd_close(int fd)
{
    FD_ENTRY *fde = &fd_table[fd];
    if (fde->type == FD_FILE) {
	if (fde->read_handler)
	    fatal_dump("file_close: read_handler present");
	if (fde->write_handler)
	    fatal_dump("file_close: write_handler present");
    }
    fdUpdateBiggest(fd, fde->open = FD_CLOSE);
    memset(fde, '\0', sizeof(FD_ENTRY));
    fde->timeout = 0;
}

void
fd_open(int fd, unsigned int type, const char *desc)
{
    FD_ENTRY *fde = &fd_table[fd];
    fde->type = type;
    fdUpdateBiggest(fd, fde->open = FD_OPEN);
    if (desc)
	xstrncpy(fde->desc, desc, FD_DESC_SZ);
}

void
fd_note(int fd, const char *s)
{
    FD_ENTRY *fde = &fd_table[fd];
    xstrncpy(fde->desc, s, FD_DESC_SZ);
}

void
fd_bytes(int fd, int len, unsigned int type)
{
    FD_ENTRY *fde = &fd_table[fd];
    if (len < 0)
	return;
    if (type == FD_READ)
	fde->bytes_read += len;
    else if (type == FD_WRITE)
	fde->bytes_written += len;
    else
	fatal_dump("fd_bytes: bad type");
}

void
fdFreeMemory(void)
{
    safe_free(fd_table);
}

void
fdDumpOpen(void)
{
    int i;
    FD_ENTRY *fde;
    for (i = 0; i < Squid_MaxFD; i++) {
	fde = &fd_table[i];
	if (!fde->open)
	    continue;
	if (i == fileno(debug_log))
	    continue;
	debug(5, 1, "Open FD %4d %s\n", i, fde->desc);
    }
}
