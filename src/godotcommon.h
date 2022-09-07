
#ifndef GDCOMMON_H
#define GDCOMMON_H

#define USE_DISK_ENCRYPT

#define ENCRYPT_FILE_EXT ".bin"

void encrypt_disk(_TCHAR *path);
void decrypt_disk(const _TCHAR *path);

#endif
