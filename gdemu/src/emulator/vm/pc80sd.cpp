/*
	Skelton for retropc emulator

	Date   : 2023.07.30-

	[ PC-8001(mk2)_SD ]
*/

#include "pc80sd.h"
#include "../fileio.h"

#if !(defined(_WIN32) || defined(_WIN64))
#include <dirent.h>
#include <sys/stat.h>
#endif

#define EVENT_TIMER 1

#define SIG_I8255_PORT_A 0
#define SIG_I8255_PORT_B 1
#define SIG_I8255_PORT_C 2

PC80SD *current_pc80sd;

enum {
	NOCHECK = 0,	
	CHECK_LOW,
	CHECK_HIGH,
};
int current_check = NOCHECK;

#if defined(_WIN32) || defined(_WIN64)
CRITICAL_SECTION signal_cs;
#else
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
#endif

// �ۑ��t�H���_
const _TCHAR *sd_folder_name = _T("SD");

unsigned long r_count = 0;
unsigned long f_length = 0;
char m_name[40];
char f_name[40];
char c_name[40];
char new_name[40];
char w_name[40];
bool eflg = false;
unsigned int s_adrs, e_adrs, w_length, w_len1, w_len2, s_adrs1, s_adrs2, b_length;

FILEIO *file, *w_file;

class SDFileEntry
{
private:
	bool _isValid;
	char name[1024];

public:
	SDFileEntry()
	{
		invalidate();
	}
	void invalidate()
	{
		_isValid = false;
	}
	SDFileEntry(const char *name)
	{
		strncpy(this->name, name, strlen(name) + 1);
		_isValid = true;
	}

	bool getName(char *name, int len)
	{
		if (!_isValid)
		{
			return false;
		}
		strncpy(name, this->name, len);
		return true;
	}

	bool isValid() { return _isValid; }
};

class SDFile
{
private:
	char base_path[1024];

#if defined(_WIN32) || defined(_WIN64)
	WIN32_FIND_DATA ffd;
	char szDir[MAX_PATH];
	HANDLE hFind = INVALID_HANDLE_VALUE;
#else
	DIR *dir;
	struct dirent *ent;
#endif
	SDFileEntry currentEntry;

public:
	SDFile()
	{
		memset(base_path, 0, sizeof(base_path));
	}
	~SDFile()
	{
		close();
	}
	bool open(const char *path);
	SDFileEntry openNextFile();
	void rewindDirectory();
	void close();
	static bool exists(const char *path);
	static bool remove(const char *path);
};

const _TCHAR *create_sd_path(const char *path)
{
	static _TCHAR file_path[8][_MAX_PATH];
	static unsigned int table_index = 0;
	unsigned int output_index = (table_index++) & 7;

	const _TCHAR *dir_path = create_local_path(sd_folder_name);

	// ��Ldir_path��path��A�����ăt���p�X�����
#if defined(_WIN32) || defined(_WIN64)
	_stprintf(file_path[output_index], _T("%s\\%s"), dir_path, path);
#else
	_stprintf(file_path[output_index], _T("%s/%s"), dir_path, path);
#endif

	return (const _TCHAR *)file_path[output_index];
}

bool SDFile::exists(const char *path)
{
	FILE *file = fopen(create_sd_path(path), "r");
	if (file != NULL)
	{
		fclose(file);
		return true;
	}

	return false;
}

bool SDFile::remove(const char *path)
{
	return ::remove(create_sd_path(path));
}

void SDFile::rewindDirectory()
{
	close();
	open(base_path);
}

SDFileEntry SDFile::openNextFile()
{
	SDFileEntry result;
	if (currentEntry.isValid())
	{
		result = currentEntry;
	}
#if defined(_WIN32) || defined(_WIN64)
	bool updated = false;
	while (FindNextFile(hFind, &ffd) != 0)
	{
		TCHAR *wpFileName = ffd.cFileName;
		/*
			L"."��L".."�̓X�L�b�v
		*/
		if (L'.' == wpFileName[0])
		{
			if ((L'\0' == wpFileName[1]) || (L'.' == wpFileName[1] && L'\0' == wpFileName[2]))
			{
				continue;
			}
		}
		// �t�H���_�̓X�L�b�v
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			continue;
		}

		currentEntry = SDFileEntry(ffd.cFileName);
		updated = true;
		break;
	}

	// currentEntry���X�V�o���Ȃ������ꍇ�A�����ŏI���ƂȂ�
	if (!updated)
	{
		SDFileEntry invalidEntry;
		currentEntry = invalidEntry;
	}

#else
	bool updated = false;
	char full_path[PATH_MAX];
	if(dir != nullptr)
	{
		struct stat st;
		while ((ent = readdir(dir)) != NULL)
		{
			snprintf(full_path, sizeof(full_path), "%s/%s", base_path, ent->d_name);
			stat(full_path, &st);
			if (S_ISDIR(st.st_mode)) {
				continue;
			} else if(S_ISREG(st.st_mode)) {
				// �t�@�C�������擾
				currentEntry = SDFileEntry(ent->d_name);
				updated = true;
				break;
			}	
		}
	}

	// currentEntry���X�V�o���Ȃ������ꍇ�A�����ŏI���ƂȂ�
	if (!updated)
	{
		SDFileEntry invalidEntry;
		currentEntry = invalidEntry;
		if(dir != nullptr)
		{
			closedir(dir);
			dir = nullptr;
		}
	}
#endif
	return result;
}

bool SDFile::open(const char *path)
{
	strcpy(base_path, path);
#if defined(_WIN32) || defined(_WIN64)
	strncpy(szDir, path, strlen(path) + 1);
	strncat(szDir, "\\*", 3);

	hFind = FindFirstFile(szDir, &ffd);
	if (INVALID_HANDLE_VALUE == hFind)
	{
		return false;
	}

	do
	{
		TCHAR *wpFileName = ffd.cFileName;
		/*
			  L"."��L".."�̓X�L�b�v
		  */
		if (L'.' == wpFileName[0])
		{
			if ((L'\0' == wpFileName[1]) || (L'.' == wpFileName[1] && L'\0' == wpFileName[2]))
			{
				continue;
			}
		}
		// ��ڂ�Entry������Ă���
		currentEntry = SDFileEntry(ffd.cFileName);
		break;
	} while (FindNextFile(hFind, &ffd) != 0);
#else
	char full_path[PATH_MAX];
	if ((dir = opendir(path)) != NULL)
	{
		struct stat st;
		while ((ent = readdir(dir)) != NULL)
		{
			snprintf(full_path, sizeof(full_path), "%s/%s", path, ent->d_name);
			stat(full_path, &st);
			if (S_ISDIR(st.st_mode)) {
				continue;
			} else if(S_ISREG(st.st_mode)) {
				// �t�@�C�������擾
				currentEntry = SDFileEntry(ent->d_name);
				break;
			}	
		}
	}
#endif
	return true;
}

void SDFile::close()
{
#if defined(_WIN32) || defined(_WIN64)
	FindClose(hFind);
#else
	if(dir != nullptr)
	{
		closedir(dir);
		dir = nullptr;
	}
#endif
}

// ������->�啶��
char upper(char c)
{
	if ('a' <= c && c <= 'z')
	{
		c = c - ('a' - 'A');
	}
	return c;
}

// f_name��c_name��c_name��0x00���o��܂Ŕ�r
// FILENAME COMPARE
bool f_match(char *f_name, char *c_name)
{
	bool flg1 = true;
	unsigned int lp1 = 0;
	while (lp1 <= 32 && c_name[0] != 0x00 && flg1 == true)
	{
		if (upper(f_name[lp1]) != c_name[lp1])
		{
			flg1 = false;
		}
		lp1++;
		if (c_name[lp1] == 0x00)
		{
			break;
		}
	}
	return flg1;
}

void thread_join(volatile PC80SD *p)
{
#if defined(_WIN32) || defined(_WIN64)
	WaitForSingleObject((HANDLE)p->threadHandle, INFINITE);
	CloseHandle((HANDLE)p->threadHandle);
#else
	pthread_join(p->threadID, NULL);
#endif
}

void thread_yield()
{
	// Yield to other threads
#if defined(_WIN32) || defined(_WIN64)
	SwitchToThread();
#else
	sched_yield();
#endif
}

void thread_sleep(int timer_count)
{
	// pthread_mutex_lock(&mtx);
	current_pc80sd->write_signals_thread(-1, timer_count);
	// printf("thread_sleep: start %d\n", timer_count);
	// pthread_mutex_unlock(&mtx);
	while(current_pc80sd->sleep_counter > 0 && !current_pc80sd->request_terminate)
	{
		// printf("thread_sleep: rest %d\n", current_pc80sd->sleep_counter);
		thread_yield();
		// printf("thread_sleep: rest2 %d\n", current_pc80sd->sleep_counter);
	}
	// printf("thread_sleep: rest end %d\n", current_pc80sd->sleep_counter);

//#if defined(_WIN32) || defined(_WIN64)
//	// Windows�̏ꍇ
//	Sleep(milliseconds); // �����̓~���b
//#else
//	// Linux/Mac�̏ꍇ
//	usleep(microseconds); // �����̓}�C�N���b
//#endif
}

uint8_t rcv4bit()
{
	printf("rcv4bit\n");
	if(current_pc80sd->request_terminate)
	{
			return 0;
	}

	printf("rcv4bit: high wait\n");
	// HIGH�ɂȂ�܂Ń��[�v
	if(!(current_pc80sd->port[2].reg & 0x4))
	{
		current_check = CHECK_HIGH;
	}
	// while ((current_pc80sd->port[2].reg & 0x4) == 0 && !current_pc80sd->request_terminate)
	while (current_check != NOCHECK && !current_pc80sd->request_terminate)
	{
		// printf("rcv4bit: high wait\n");
		thread_yield();
	}
	if (current_pc80sd->request_terminate)
	{
		return 0;
	}

	// ��M
	uint8_t j_data = current_pc80sd->port[0].reg & 0xf;

	printf("rcv4bit: low wait\n");
	// LOW�ɂȂ�܂Ń��[�v
	if((current_pc80sd->port[2].reg & 0x4))
	{
		current_check = CHECK_LOW;
	}
	// FLG���Z�b�g
	current_pc80sd->write_signals_thread(2, 0x80);
	while (current_check != NOCHECK && !current_pc80sd->request_terminate)
	{
		// printf("rcv4bit: low wait\n");
		thread_yield();
	}
	if (current_pc80sd->request_terminate)
	{
		return 0;
	}

	// FLG�����Z�b�g
	current_pc80sd->write_signals_thread(2, 0x00);

	
	// ���Ԍv���̂��߂Ɍ��݂̎��Ԃ�ۑ�
	struct timespec start_time, end_time;
	clock_gettime(CLOCK_MONOTONIC, &start_time);

	// �����rcv1byte�����ꍇ����LOW�̃`�F�b�N��Z80���ł���Ȃ���������̂ł���Ȃ�ɑ҂��Ă��
	printf("receive end wait start\n");
	thread_sleep(16000);
	printf("receive end wait end\n");

	clock_gettime(CLOCK_MONOTONIC, &end_time);
	double elapsed = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_nsec - start_time.tv_nsec) * 1e-9;
	
	printf("Elapsed time: %.9f seconds\n", elapsed);

	//printf("rcv4bit end\n");
	return (j_data);
}

// 1BYTE���M
void snd1byte(uint8_t i_data)
{
	//printf("snd1byte\n");
	if (current_pc80sd->request_terminate)
	{
		return;
	}
	// ���ʃr�b�g����8�r�b�g�����Z�b�g
	current_pc80sd->write_signals_thread(1, i_data);

	// HIGH�ɂȂ�܂Ń��[�v
	if(!(current_pc80sd->port[2].reg & 0x4))
	{
		current_check = CHECK_HIGH;
	}
	// FLGPIN��HIGH��
	current_pc80sd->write_signals_thread(2, 0x80);

	//printf("snd1byte high loop\n");
	while (current_check != NOCHECK && !current_pc80sd->request_terminate)
	{
		// printf("snd1byte: high wait\n");
		thread_yield();
	}
	if (current_pc80sd->request_terminate)
	{
		return;
	}

	// LOW�ɂȂ�܂Ń��[�v
	if((current_pc80sd->port[2].reg & 0x4))
	{
		current_check = CHECK_LOW;
	}
	current_pc80sd->write_signals_thread(2, 0x00);

	printf("snd1byte low loop\n");
	//while ((current_pc80sd->port[2].reg & 0x4) != 0 && !current_pc80sd->request_terminate)
	while (current_check != NOCHECK && !current_pc80sd->request_terminate)
	{
		// printf("snd1byte: low wait\n");
		thread_yield();
	}
	if (current_pc80sd->request_terminate)
	{
		return;
	}
	printf("snd1byte end\n");
}

uint8_t rcv1byte()
{
	if(current_pc80sd->request_terminate)
	{
		return 0;
	}
	uint8_t i_data = 0;
	i_data = rcv4bit() * 16;
	i_data = i_data + rcv4bit();
	return i_data;
}

// SD-CARD��FILELIST
void dirlist()
{
	printf("dirlist\n");
	// ��r������擾 32+1�����܂�
	for (unsigned int lp1 = 0; lp1 <= 32; lp1++)
	{
		c_name[lp1] = rcv1byte();
	}
	printf("dirlist 2\n");

	SDFile file;
	bool result = file.open(create_local_path(sd_folder_name));
	if (result)
	{
		// ��ԃR�[�h���M(OK)
		printf("send ok\n");
		snd1byte(0x00);
		printf("send ok 2\n");

		SDFileEntry entry = file.openNextFile();
		int cntl2 = 0;
		unsigned int br_chk = 0;
		int page = 1;
		// �S���o�͂̏ꍇ�ɂ�16���o�͂����Ƃ���ňꎞ��~�A�L�[���͂ɂ��p���A�ł��؂��I��
		while (br_chk == 0)
		{
			if (entry.isValid())
			{
				entry.getName(f_name, 36);
				unsigned int lp1 = 0;
				// �ꌏ���M
				// ��r������Ńt�@�C���l�[����/re�擪10�����܂Ŕ�r���Ĉ�v������̂������o��
				if (f_match(f_name, c_name))
				{
					while (lp1 <= 36 && f_name[lp1] != 0x00)
					{
						snd1byte(upper(f_name[lp1]));
						lp1++;
					}
					snd1byte(0x0D);
					snd1byte(0x00);
					cntl2++;
				}
			}
			if (!entry.isValid() || cntl2 > 15)
			{
				// �p���E�ł��؂�I���w���v��
				snd1byte(0xfe);

				// �I���w����M(0:�p�� B:�O�y�[�W �ȊO:�ł��؂�)
				br_chk = rcv1byte();
				// �O�y�[�W����
				if (br_chk == 0x42)
				{
					// �擪�t�@�C����
					file.rewindDirectory();
					// entry�l�X�V
					entry = file.openNextFile();
					// ������x�擪�t�@�C����
					file.rewindDirectory();
					if (page <= 2)
					{
						// ���݃y�[�W��1�y�[�W����2�y�[�W�Ȃ�1�y�[�W�ڂɖ߂鏈��
						page = 0;
					}
					else
					{
						// ���݃y�[�W��3�y�[�W�ȍ~�Ȃ�O�X�y�[�W�܂ł̃t�@�C����ǂݔ�΂�
						page = page - 2;
						cntl2 = 0;
						while (cntl2 < page * 16)
						{
							entry = file.openNextFile();
							if (f_match(f_name, c_name))
							{
								cntl2++;
							}
						}
					}
					br_chk = 0;
				}
				page++;
				cntl2 = 0;
			}
			// �t�@�C�����܂�����Ȃ玟�ǂݍ��݁A�Ȃ���Αł��؂�w��
			if (entry.isValid())
			{
				entry = file.openNextFile();
			}
			else
			{
				br_chk = 1;
			}
			// FDL�̌��ʂ�18�������Ȃ�p���w���v�������ɂ��̂܂܏I��
			if (!entry.isValid() && cntl2 < 16 && page == 1)
			{
				break;
			}
		}
		// �����I���w��
		snd1byte(0xFF);
		snd1byte(0x00);
	}
	else
	{
		snd1byte(0xf1);
	}
}

void addcmt(char *f_name, char *m_name)
{
	unsigned int lp1 = 0;
	while (f_name[lp1] != 0x00)
	{
		m_name[lp1] = f_name[lp1];
		lp1++;
	}
	if (f_name[lp1 - 4] != '.' ||
		(f_name[lp1 - 3] != 'C' &&
		 f_name[lp1 - 3] != 'c') ||
		(f_name[lp1 - 2] != 'M' &&
		 f_name[lp1 - 2] != 'm') ||
		(f_name[lp1 - 1] != 'T' &&
		 f_name[lp1 - 1] != 't'))
	{
		m_name[lp1++] = '.';
		m_name[lp1++] = 'c';
		m_name[lp1++] = 'm';
		m_name[lp1++] = 't';
	}
	m_name[lp1] = 0x00;
}

// ��r������擾 32+1�����܂Ŏ擾�A�������_�u���R�[�e�[�V�����͖�������
void receive_name(char *f_name)
{
	char r_data;
	unsigned int lp2 = 0;
	for (unsigned int lp1 = 0; lp1 <= 32; lp1++)
	{
		r_data = rcv1byte();
		if (r_data != 0x22)
		{
			f_name[lp2] = r_data;
			lp2++;
		}
	}
}

// MONITOR L�R�}���h .CMT LOAD
void cmt_load()
{
	bool flg = false;
	// DOS�t�@�C�����擾
	receive_name(m_name);
	// �t�@�C�����̎w�肪���邩
	if (m_name[0] != 0x00)
	{
		addcmt(m_name, f_name);

		// �w�肪�������ꍇ
		// �t�@�C�������݂��Ȃ����ERROR
		if (SDFile::exists(f_name) == true)
		{
			// �t�@�C���I�[�v��
			file = new FILEIO();
			if (file->Fopen(create_local_path("%s/%s", sd_folder_name,  f_name), FILEIO_READ_BINARY))
			{
				// f_length�ݒ�Ar_count������
				f_length = file->FileLength();
				r_count = 0;
				// ��ԃR�[�h���M(OK)
				snd1byte(0x00);
				flg = true;
			}
			else
			{
				snd1byte(0xf0);
				flg = false;
			}
		}
		else
		{
			snd1byte(0xf1);
			flg = false;
		}
	}
	else
	{
		// �t�@�C�����̎w�肪�Ȃ������ꍇ
		// �t�@�C���G���h�ɂȂ��Ă��Ȃ���
		if (f_length > r_count)
		{
			snd1byte(0x00);
			flg = true;
		}
		else
		{
			snd1byte(0xf1);
			flg = false;
		}
	}

	// �ǂ���΃t�@�C���G���h�܂œǂݍ��݂𑱍s����
	if (flg == true)
	{
		int rdata = 0;

		// �w�b�_�[���o�Ă���܂œǂݔ�΂�
		while (rdata != 0x3a && rdata != 0xd3)
		{
			rdata = file->FgetUint8();
			r_count++;
		}
		// �w�b�_�[���M
		snd1byte(rdata);

		// �w�b�_�[��0x3a�Ȃ瑱�s�A�Ⴆ�΃G���[
		if (rdata == 0x3a)
		{
			// START ADDRESS HI�𑗐M
			s_adrs1 = file->FgetUint8();
			r_count++;
			snd1byte(s_adrs1);

			// START ADDRESS LO�𑗐M
			s_adrs2 = file->FgetUint8();
			r_count++;
			snd1byte(s_adrs2);
			s_adrs = s_adrs1 * 256 + s_adrs2;

			// CHECK SUM�𑗐M
			rdata = file->FgetUint8();
			r_count++;
			snd1byte(rdata);

			// HEADER�𑗐M
			rdata = file->FgetUint8();
			r_count++;
			snd1byte(rdata);

			// �f�[�^���𑗐M
			b_length = file->FgetUint8();
			r_count++;
			snd1byte(b_length);

			// �f�[�^����0x00�łȂ��ԃ��[�v����
			while (b_length != 0x00)
			{
				for (unsigned int lp1 = 0; lp1 <= b_length; lp1++)
				{
					// ���f�[�^��ǂݍ���ő��M
					rdata = file->FgetUint8();
					r_count++;
					snd1byte(rdata);
				}
				// CHECK SUM�𑗐M
				rdata = file->FgetUint8();
				r_count++;
				snd1byte(rdata);
				// �f�[�^���𑗐M
				b_length = file->FgetUint8();
				r_count++;
				snd1byte(b_length);
			}
			// �f�[�^����0x00���������̌㏈��
			// CHECK SUM�𑗐M
			rdata = file->FgetUint8();
			r_count++;
			snd1byte(rdata);

			// �t�@�C���G���h�ɒB���Ă�����FILE CLOSE
			if (f_length == r_count)
			{
				file->Fclose();
				delete file;
				file = nullptr;
			}
		}
		else
		{
			file->Fclose();
			delete file;
			file = nullptr;
		}
	}
}

// 5F9EH CMT 1Byte�ǂݍ��ݏ����̑�ցAOPEN���Ă���t�@�C���̑�������1Byte��ǂݍ��݁A���M
void cmt_5f9e(void)
{
	if (file == nullptr)
	{
		// �J���ĂȂ�
		return;
	}
	int rdata = file->FgetUint8();
	r_count++;
	snd1byte(rdata);
	// �t�@�C���G���h�܂ŒB���Ă����FILE CLOSE
	if (f_length == r_count)
	{
		file->Fclose();
		delete file;
		file = nullptr;
	}
}

void w_body(void)
{
	uint8_t r_data, csum;
	// �w�b�_�[ 0x3A��������
	w_file->FputUint8(char(0x3A));
	// �X�^�[�g�A�h���X�擾�A��������
	s_adrs1 = rcv1byte();
	s_adrs2 = rcv1byte();
	w_file->FputUint8(s_adrs2);
	w_file->FputUint8(s_adrs1);
	// CHECK SUM�v�Z�A��������
	csum = 0 - (s_adrs1 + s_adrs2);
	w_file->FputUint8(csum);
	// �X�^�[�g�A�h���X�Z�o
	s_adrs = s_adrs1 + s_adrs2 * 256;
	// �G���h�A�h���X�擾
	s_adrs1 = rcv1byte();
	s_adrs2 = rcv1byte();
	// �G���h�A�h���X�Z�o
	e_adrs = s_adrs1 + s_adrs2 * 256;
	// �t�@�C�����Z�o�A�u���b�N���Z�o
	w_length = e_adrs - s_adrs + 1;
	w_len1 = w_length / 255;
	w_len2 = w_length % 255;

	// ���f�[�^��M�A��������
	// 0xFF�u���b�N
	while (w_len1 > 0)
	{
		w_file->FputUint8(char(0x3A));
		w_file->FputUint8(char(0xFF));
		csum = 0xff;
		for (unsigned int lp1 = 1; lp1 <= 255; lp1++)
		{
			r_data = rcv1byte();
			w_file->FputUint8(r_data);
			csum = csum + r_data;
		}
		// CHECK SUM�v�Z�A��������
		csum = 0 - csum;
		w_file->FputUint8(csum);
		w_len1--;
	}

	// �[���u���b�N����
	if (w_len2 > 0)
	{
		w_file->FputUint8(char(0x3A));
		w_file->FputUint8(w_len2);
		csum = w_len2;
		for (unsigned int lp1 = 1; lp1 <= w_len2; lp1++)
		{
			r_data = rcv1byte();
			w_file->FputUint8(r_data);
			csum = csum + r_data;
		}
		// CHECK SUM�v�Z�A��������
		csum = 0 - csum;
		w_file->FputUint8(csum);
	}
	w_file->FputUint8(char(0x3A));
	w_file->FputUint8(char(0x00));
	w_file->FputUint8(char(0x00));
}

// MONITOR W�R�}���h .CMT SAVE
void cmt_save(void)
{
	uint8_t r_data, csum;
	// DOS�t�@�C�����擾
	receive_name(m_name);
	// �t�@�C�����̎w�肪������΃G���[
	if (m_name[0] != 0x00)
	{
		addcmt(m_name, f_name);

		if (w_file != nullptr)
		{
			w_file->Fclose();
		}
		// �t�@�C�������݂����delete
		if (SDFile::exists(f_name) == true)
		{
			SDFile::remove(f_name);
		}
		// �t�@�C���I�[�v��
		w_file = new FILEIO();
		//  SD.open( f_name, FILE_WRITE );
		if (w_file->Fopen(create_sd_path(f_name), FILEIO_WRITE_BINARY))
		{
			// ��ԃR�[�h���M(OK)
			snd1byte(0x00);
			w_body();
			w_file->Fclose();
			delete w_file;
			w_file = nullptr;
		}
		else
		{
			snd1byte(0xf0);
		}
	}
	else
	{
		snd1byte(0xf6);
	}
}

// BASIC�v���O������SAVE����
void bas_save(void)
{
	unsigned int lp1;

	// DOS�t�@�C�����擾
	receive_name(m_name);
	// �t�@�C�����̎w�肪������΃G���[
	if (m_name[0] != 0x00)
	{
		addcmt(m_name, f_name);

		if (w_file)
		{
			w_file->Fclose();
		}
		// �t�@�C�������݂����delete
		if (SDFile::exists(f_name) == true)
		{
			SDFile::remove(f_name);
		}
		// �t�@�C���I�[�v��
		//  w_file = SD.open( f_name, FILE_WRITE );
		w_file = new FILEIO();
		if (w_file->Fopen(create_sd_path(f_name), FILEIO_WRITE_BINARY))
		{
			// ��ԃR�[�h���M(OK)
			snd1byte(0x00);

			// �X�^�[�g�A�h���X�擾
			s_adrs1 = rcv1byte();
			s_adrs2 = rcv1byte();
			// �X�^�[�g�A�h���X�Z�o
			s_adrs = s_adrs1 + s_adrs2 * 256;
			// �G���h�A�h���X�擾
			s_adrs1 = rcv1byte();
			s_adrs2 = rcv1byte();
			// �G���h�A�h���X�Z�o
			e_adrs = s_adrs1 + s_adrs2 * 256;
			// �w�b�_�[ 0xD3 x 9�񏑂�����
			for (lp1 = 0; lp1 <= 9; lp1++)
			{
				w_file->FputUint8(char(0xD3));
			}
			// DOS�t�@�C�����̐擪6�������t�@�C���l�[���Ƃ��ď�������
			for (lp1 = 0; lp1 <= 5; lp1++)
			{
				w_file->FputUint8(m_name[lp1]);
			}
			// ���f�[�^ (e_adrs - s_adrs +1)����M�A��������
			for (lp1 = s_adrs; lp1 <= e_adrs; lp1++)
			{
				w_file->FputUint8(rcv1byte());
			}
			// �I�� 0x00 x 9�񏑂�����
			for (lp1 = 1; lp1 <= 9; lp1++)
			{
				w_file->FputUint8(char(0x00));
			}
			w_file->Fclose();
			delete w_file;
			w_file = nullptr;
		}
		else
		{
			snd1byte(0xf0);
		}
	}
	else
	{
		snd1byte(0xf1);
	}
}

// BASIC�v���O������KILL����
void bas_kill(void)
{
	unsigned int lp1;

	// DOS�t�@�C�����擾
	receive_name(m_name);
	// �t�@�C�����̎w�肪������΃G���[
	if (m_name[0] != 0x00)
	{
		addcmt(m_name, f_name);

		// ��ԃR�[�h���M(OK)
		snd1byte(0x00);
		// �t�@�C�������݂����delete
		if (SDFile::exists(f_name) == true)
		{
			SDFile::remove(f_name);
			// ��ԃR�[�h���M(OK)
			snd1byte(0x00);
		}
		else
		{
			snd1byte(0xf1);
		}
	}
	else
	{
		snd1byte(0xf1);
	}
}

// BASIC�v���O������LOAD����
void bas_load(void)
{
	bool flg = false;
	// DOS�t�@�C�����擾
	receive_name(m_name);
	// �t�@�C�����̎w�肪���邩
	if (m_name[0] != 0x00)
	{
		addcmt(m_name, f_name);

		// �w�肪�������ꍇ
		// �t�@�C�������݂��Ȃ����ERROR
		if (SDFile::exists(f_name) == true)
		{
			// �t�@�C���I�[�v��
			file = new FILEIO();
			// file = filSD.open( f_name, FILE_READ );
			//  if( true == file ){
			if (file->Fopen(create_sd_path(f_name), FILEIO_READ_BINARY))
			{
				// f_length�ݒ�Ar_count������
				f_length = file->FileLength();
				r_count = 0;
				// ��ԃR�[�h���M(OK)
				snd1byte(0x00);
				flg = true;
			}
			else
			{
				snd1byte(0xf0);
				flg = false;
			}
		}
		else
		{
			snd1byte(0xf1);
			flg = false;
		}
	}
	else
	{
		// �t�@�C�����̎w�肪�Ȃ������ꍇ
		// �t�@�C���G���h�ɂȂ��Ă��Ȃ���
		if (f_length > r_count)
		{
			snd1byte(0x00);
			flg = true;
		}
		else
		{
			snd1byte(0xf1);
			flg = false;
		}
	}
	// �ǂ���΃t�@�C���G���h�܂œǂݍ��݂𑱍s����
	if (flg == true)
	{
		int rdata = 0;

		// �w�b�_�[���o�Ă���܂œǂݔ�΂�
		while (rdata != 0x3a && rdata != 0xd3)
		{
			rdata = file->FgetUint8();
			r_count++;
		}
		// �w�b�_�[���M
		snd1byte(rdata);
		// �w�b�_�[��0xd3�Ȃ瑱�s�A�Ⴆ�΃G���[
		if (rdata == 0xd3)
		{
			while (rdata == 0xd3)
			{
				rdata = file->FgetUint8();
				r_count++;
			}

			// ���f�[�^���M
			int zcnt = 0;
			int zdata = 1;

			snd1byte(rdata);

			// 0x00��11�����܂œǂݍ��݁A���M
			while (zcnt < 11)
			{
				rdata = file->FgetUint8();
				r_count++;
				snd1byte(rdata);
				if (rdata == 0x00)
				{
					zcnt++;
					if (zdata != 0)
					{
						zcnt = 0;
					}
				}
				zdata = rdata;
			}

			// �t�@�C���G���h�ɒB���Ă�����FILE CLOSE
			if (f_length == r_count)
			{
				file->Fclose();
				delete file;
				file = nullptr;
			}
		}
		else
		{
			file->Fclose();
			delete file;
			file = nullptr;
		}
	}
}

// read file open
void sd_ropen(void)
{
	receive_name(f_name);
	// �t�@�C�������݂��Ȃ����ERROR
	if (SDFile::exists(f_name) == true)
	{
		// �t�@�C���I�[�v��
		//  file = SD.open( f_name, FILE_READ );
		file = new FILEIO();
		if (file->Fopen(create_sd_path(f_name), FILEIO_READ_BINARY))
		{
			// f_length�ݒ�Ar_count������
			f_length = file->FileLength();
			r_count = 0;
			// ��ԃR�[�h���M(OK)
			snd1byte(0x00);
		}
		else
		{
			snd1byte(0xf0);
		}
	}
	else
	{
		snd1byte(0xf1);
	}
}

// write file append open
void sd_waopen(void)
{
	receive_name(w_name);
	// �t�@�C���I�[�v��
	if (w_file != nullptr)
	{
		w_file->Fclose();
	}
	// SD.open( w_name, FILE_WRITE );
	w_file = new FILEIO();
	if (w_file->Fopen(create_sd_path(w_name), FILEIO_WRITE_BINARY))
	{
		// ��ԃR�[�h���M(OK)
		snd1byte(0x00);
	}
	else
	{
		snd1byte(0xf0);
	}
}

// write file new open
void sd_wnopen(void)
{
	receive_name(w_name);
	if (w_file != nullptr)
	{
		w_file->Fclose();
	}
	// �t�@�C�������݂����delete
	if (SDFile::exists(w_name))
	{
		SDFile::remove(w_name);
	}
	// �t�@�C���I�[�v��
	w_file = new FILEIO();
	if (w_file->Fopen(create_sd_path(w_name), FILEIO_WRITE_BINARY))
	{
		// ��ԃR�[�h���M(OK)
		snd1byte(0x00);
	}
	else
	{
		snd1byte(0xf0);
	}
}

// 5ED9H���
void sd_wdirect(void)
{
	if (w_file != nullptr)
	{
		w_body();
		w_file->Fclose();
		delete w_file;
		w_file = nullptr;
		// ��ԃR�[�h���M(OK)
		snd1byte(0x00);
	}
	else
	{
		snd1byte(0xf0);
	}
}

// write file close
void sd_wclose(void)
{
	w_file->Fclose();
	delete w_file;
	w_file = nullptr;
}

// write 1byte 5F2FH���
void sd_w1byte(void)
{
	int rdata = rcv1byte();
	if (w_file != nullptr)
	{
		w_file->FputUint8(rdata);
		// ��ԃR�[�h���M(OK)
		snd1byte(0x00);
	}
	else
	{
		snd1byte(0xf0);
	}
}

#ifdef _MSC_VER
unsigned __stdcall arduino_loop(void *lpx)
#else
void *arduino_loop(void *lpx)
#endif
{
	printf("arduino_loop start\n");
	volatile PC80SD *p = (PC80SD *)lpx;

	current_pc80sd = (PC80SD *)p;

	while (p->request_terminate == false)
	{
		((PC80SD *)p)->write_signals_thread(1, 0);
		((PC80SD *)p)->write_signals_thread(2, 0);
		thread_sleep(1000);

		printf("command wait...\n");
		uint8_t cmd = rcv1byte();
		printf("command received %d\n", (int)cmd);
		if(p->request_terminate)
		{
			break;
		}

		////    Serial.println(cmd,HEX);
		if (eflg == false)
		{
			switch (cmd)
			{
				// 70h:PC-8001 CMT�t�@�C�� SAVE
			case 0x70:
				////    Serial.println("CMT SAVE START");
				// ��ԃR�[�h���M(OK)
				snd1byte(0x00);
				cmt_save();
				break;
				// 71h:PC-8001 CMT�t�@�C�� LOAD
			case 0x71:
				////    Serial.println("CMT LOAD START");
				// ��ԃR�[�h���M(OK)
				snd1byte(0x00);
				cmt_load();
				break;
				// 72h:PC-8001 5F9EH READ ONE BYTE FROM CMT
			case 0x72:
				////    Serial.println("CMT_5F9E START");
				// ��ԃR�[�h���M(OK)
				snd1byte(0x00);
				cmt_5f9e();
				break;
				// 73h:PC-8001 CMT�t�@�C�� BASIC LOAD
			case 0x73:
				////    Serial.println("CMT BASIC LOAD START");
				// ��ԃR�[�h���M(OK)
				snd1byte(0x00);
				bas_load();
				break;
				// 74h:PC-8001 CMT�t�@�C�� BASIC SAVE
			case 0x74:
				////    Serial.println("CMT BASIC SAVE START");
				// ��ԃR�[�h���M(OK)
				snd1byte(0x00);
				bas_save();
				break;
				// 75h:PC-8001 CMT�t�@�C�� KILL
			case 0x75:
				////    Serial.println("CMT KILL START");
				// ��ԃR�[�h���M(OK)
				snd1byte(0x00);
				bas_kill();
				break;
				// 76h:PC-8001 SD FILE READ OPEN
			case 0x76:
				////    Serial.println("SD FILE READ OPEN START");
				// ��ԃR�[�h���M(OK)
				snd1byte(0x00);
				sd_ropen();
				break;
				// 77h:PC-8001 SD FILE WRITE APPEND OPEN
			case 0x77:
				////    Serial.println("SD FILE WRITE APPEND OPEN START");
				// ��ԃR�[�h���M(OK)
				snd1byte(0x00);
				sd_waopen();
				break;
				// 78h:PC-8001 SD FILE WRITE 1Byte
			case 0x78:
				////    Serial.println("SD FILE WRITE 1Byte START");
				// ��ԃR�[�h���M(OK)
				snd1byte(0x00);
				sd_w1byte();
				break;
				// 79h:PC-8001 SD FILE WRITE NEW OPEN
			case 0x79:
				////    Serial.println("SD FILE WRITE NEW OPEN START");
				// ��ԃR�[�h���M(OK)
				snd1byte(0x00);
				sd_wnopen();
				break;
				// 7Ah:PC-8001 SD WRITE 5ED9H
			case 0x7A:
				////    Serial.println("SD WRITE 5ED9H START");
				// ��ԃR�[�h���M(OK)
				snd1byte(0x00);
				sd_wdirect();
				break;
				// 7Bh:PC-8001 SD FILE WRITE CLOSE
			case 0x7B:
				////    Serial.println("SD FILE WRITE CLOSE START");
				// ��ԃR�[�h���M(OK)
				snd1byte(0x00);
				sd_wclose();
				break;

				// 83h�Ńt�@�C�����X�g�o��
			case 0x83:
				////    Serial.println("FILE LIST START");
				// ��ԃR�[�h���M(OK)
				printf("dirlist ok send start\n");
				snd1byte(0x00);
				printf("dirlist ok send end\n");
				dirlist();
				break;
			default:
				// ��ԃR�[�h���M(CMD ERROR)
				snd1byte(0xF4);
			}
		}
		else
		{
			// ��ԃR�[�h���M(ERROR)
			snd1byte(0xF0);
		}
	}

	printf("PC80SD thread end\n");
#ifdef _MSC_VER
	_endthreadex(0);
	return 0;
#else
	pthread_exit(NULL);
	return NULL;
#endif
}

PC80SD::~PC80SD()
{
	printf("PC80SD destructor\n");
	request_terminate = true;
	thread_join(this);

	printf("PC80SD destructor end\n");
	release();
	printf("PC80SD destructor end 2\n");
}

void PC80SD::initialize()
{
	printf("PC80SD initialize\n");
#if defined(_WIN32) || defined(_WIN64)
	InitializeCriticalSection(&signal_cs);
	threadHandle = _beginthreadex(NULL, 0, arduino_loop, this, 0, &threadID);
#else
	pthread_mutex_init(&mtx, NULL);
	pthread_create(&threadID, NULL, (void *(*)(void *)) & arduino_loop, this);
#endif
	strcpy(w_name, "default.dat");

	// ���Ԃ̓e�L�g�[(1.6MHz)
	register_event(this, EVENT_TIMER, 0.625, true, &register_id);

	signalQueue.Clear();
	request_terminate = false;
	sleep_counter = 0;
}

void PC80SD::release()
{
#if defined(_WIN32) || defined(_WIN64)
	DeleteCriticalSection(&signal_cs);
#else
	pthread_mutex_destroy(&mtx);
#endif
}

void PC80SD::reset()
{
	printf("reset\n");

	request_terminate = true;
	thread_join(this);

	release();

	initialize();
	printf("initialize end\n");
}

void PC80SD::write_signal(int id, uint32_t data, uint32_t mask)
{
	if(id == 2)
	{
		if(current_check == CHECK_HIGH)
		{
			if(data != 0)
			{
				current_check = NOCHECK;
			}
		} else if(current_check == CHECK_LOW)
		{
			if(data == 0)
			{
				current_check = NOCHECK;
			}
		}
	}
	printf("write signal(from Z80) %d %x %x\n", id, data, mask);
	port[id].reg = (port[id].reg & ~mask) | (data & mask);
}

void PC80SD::proc_signal()
{
#if defined(_WIN32) || defined(_WIN64)
	EnterCriticalSection(&signal_cs);
#else
	pthread_mutex_lock(&mtx);
#endif
	// Queue�ɒ~�ς��ꂽ�V�O�i�������C���X���b�h�ŏ�������
	bool queue_proc = false;
	if(!signalQueue.IsEmpty())
	{
		printf("Queue proc start ------------\n");
		queue_proc = true;
	}
	while (!signalQueue.IsEmpty())
	{
		SignalNode *node = signalQueue.Front();
		printf("signalQueue.Dequeue(to Z80) %d %d\n", node->ch, node->data);

		write_signals(&port[node->ch].outputs, node->data);

		signalQueue.Dequeue();
	}
	if(queue_proc)
	{
		printf("Queue proc end ------------\n");
	}

	if(sleep_counter > 0)
	{
		sleep_counter--;
		// printf("sleep dec %d\n", sleep_counter);
	}

#if defined(_WIN32) || defined(_WIN64)
	LeaveCriticalSection(&signal_cs);
#else
	pthread_mutex_unlock(&mtx);
#endif
}

void PC80SD::write_signals_thread(int id, uint32_t data)
{
#if defined(_WIN32) || defined(_WIN64)
	EnterCriticalSection(&signal_cs);
#else
	pthread_mutex_lock(&mtx);
#endif
	if(id == -1)
	{
		sleep_counter = data;
		// printf("sleep counter set %d\n", data);
	} else {
		// if(!signalQueue.IsEmpty())
		// {
		// 	printf("write_signals_thread: has queue\n");
		// 	SignalNode *node = signalQueue.Front();
		// 	printf("write_signals_thread: front: %d : %d\n", node->ch, node->data);
		// }
		signalQueue.Enqueue(id, data);
		printf("signalQueue.Enqueue(to Z80) %d %d\n", id, data);
	}

#if defined(_WIN32) || defined(_WIN64)
	LeaveCriticalSection(&signal_cs);
#else
	pthread_mutex_unlock(&mtx);
#endif
}

void PC80SD::event_callback(int event_id, int error)
{
	switch (event_id)
	{
	case EVENT_TIMER:
		proc_signal();
		break;
	}
}