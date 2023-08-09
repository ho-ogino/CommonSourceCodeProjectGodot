/*
	Skelton for retropc emulator

	Date   : 2023.07.30-

	[ PC-8001(mk2)_SD ]
*/


#include "pc80sd.h"
#include "../fileio.h"


#define EVENT_TIMER 1

#define SIG_I8255_PORT_A 0
#define SIG_I8255_PORT_B 1
#define SIG_I8255_PORT_C 2

PC80SD *current_pc80sd;

static uint8_t cr_data[2] = {0x0d,0x00};
static uint8_t end_data[2] = {0xff,0x00};

enum pc80sd_state
{
	PC80SD_INITIALIZE = 0,
	PC80SD_WAIT_COMMAND,

	PC80SD_RCV4BIT,
	PC80SD_RCV1BYTE,
	PC80SD_SND1BYTE,
	PC80SD_RCVBYTES,
	PC80SD_SNDBYTES,
	PC80SD_RCVNAME,
	PC80SD_WBODY,

	PC80SD_DIRLIST = 0x83,
	PC80SD_CMT_SAVE = 0x70,
	PC80SD_CMT_LOAD = 0x71,
	PC80SD_CMT_5F9E = 0x72,
	PC80SD_BAS_LOAD = 0x73,
	PC80SD_BAS_SAVE = 0x74,
	PC80SD_BAS_KILL = 0x75,
	PC80SD_SD_ROPEN = 0x76,
	PC80SD_SD_WAOPEN = 0x77,
	PC80SD_SD_W1BYTE = 0x78,
	PC80SD_SD_WNOPEN = 0x79,
	PC80SD_SD_WDIRECT = 0x7A,
	PC80SD_SD_WCLOSE = 0x7B,

	PC80SD_WAIT,
};

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

FILEIO *file = nullptr, *w_file = nullptr;

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
	if(hFind != INVALID_HANDLE_VALUE)
	{
		FindClose(hFind);
		hFind = INVALID_HANDLE_VALUE;
	}
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

PC80SD::~PC80SD()
{
	release();
}

void PC80SD::initialize()
{
	strcpy(w_name, "default.dat");

	// ���Ԃ̓e�L�g�[(1.6MHz)
	register_event(this, EVENT_TIMER, 0.625, true, &register_id);

	stateStack.clear();
	state = PC80SD_INITIALIZE;
	sub_state = 0;
	memset(m_name, 0, sizeof(m_name));
	memset(f_name, 0, sizeof(f_name));
	memset(c_name, 0, sizeof(c_name));
	memset(new_name, 0, sizeof(new_name));
	memset(w_name, 0, sizeof(w_name));
	r_count = f_length = 0;
}

void PC80SD::release()
{
	if (w_file)
	{
		w_file->Fclose();
		w_file = nullptr;
	}
	if (file)
	{
		file->Fclose();
		file = nullptr;
	}
	sd_file.close();
}

void PC80SD::reset()
{
	release();

	initialize();
}

void PC80SD::write_signal(int id, uint32_t data, uint32_t mask)
{
	// printf("write signal(from Z80) %d %x %x\n", id, data, mask);
	port[id].reg = (port[id].reg & ~mask) | (data & mask);
}

void PC80SD::sendbytes(const uint8_t*data, int sz)
{
	stateStack.push(state);
	stateStack.push(sub_state);
	state = PC80SD_SNDBYTES;
	sub_state = 0;
	memcpy(send_datas, data, sz);
	send_datas_count = sz;
	send_datas_index = 0;
}

void PC80SD::receivebytes(int count)
{
	stateStack.push(state);
	stateStack.push(sub_state);
	state = PC80SD_RCVBYTES;
	sub_state = 0;
	receive_datas_count = count;
	receive_datas_index = 0;
}

void PC80SD::receive1byte()
{
	stateStack.push(state);
	stateStack.push(sub_state);
	state = PC80SD_RCV1BYTE;
	sub_state = 0;
	result_value = 0;
}

void PC80SD::receive4bit()
{
	stateStack.push(state);
	stateStack.push(sub_state);
	state = PC80SD_RCV4BIT;
	sub_state = 0;
}

void PC80SD::popstate()
{
	sub_state = stateStack.top();
	stateStack.pop();
	state = stateStack.top();
	stateStack.pop();
}

void PC80SD::proc_command()
{
}

void PC80SD::send1byte(uint8_t data)
{
	stateStack.push(state);
	stateStack.push(sub_state);
	state = PC80SD_SND1BYTE;
	sub_state = 0;
	send_value = data;
}

void PC80SD::loopend()
{
	state = PC80SD_INITIALIZE;
	sub_state = 0;
}

void PC80SD::receivename(char *f_name)
{
	stateStack.push(state);
	stateStack.push(sub_state);
	state = PC80SD_RCVNAME;
	sub_state = 0;
	target_data = f_name;
}

void PC80SD::w_body(void)
{
	stateStack.push(state);
	stateStack.push(sub_state);
	state = PC80SD_WBODY;
	sub_state = 0;
}

void PC80SD::w_body_proc()
{
	switch(sub_state)
	{
		case 0:
		{
			// �w�b�_�[ 0x3A��������
			w_file->FputUint8(char(0x3A));

			sub_state++;
			receivebytes(2);
			break;
		}
		case 1:
		{
			// �X�^�[�g�A�h���X�擾�A��������
			s_adrs1 = receive_datas[0];
			s_adrs2 = receive_datas[1];
			w_file->FputUint8(s_adrs2);
			w_file->FputUint8(s_adrs1);
			// CHECK SUM�v�Z�A��������
			csum = 0 - (s_adrs1 + s_adrs2);
			w_file->FputUint8(csum);
			// �X�^�[�g�A�h���X�Z�o
			s_adrs = s_adrs1 + s_adrs2 * 256;

			sub_state++;
			receivebytes(2);
			break;
		}
		case 2:
		{
			// �G���h�A�h���X�擾
			s_adrs1 = receive_datas[0];
			s_adrs2 = receive_datas[1];
			// �G���h�A�h���X�Z�o
			e_adrs = s_adrs1 + s_adrs2 * 256;
			// �t�@�C�����Z�o�A�u���b�N���Z�o
			w_length = e_adrs - s_adrs + 1;
			w_len1 = w_length / 255;
			w_len2 = w_length % 255;

			sub_state++;
			break;
		}
		case 3:
		{
			// ���f�[�^��M�A��������
			// 0xFF�u���b�N
			if(w_len1 == 0)
			{
				sub_state = 100;
				break;
			}
			w_file->FputUint8(char(0x3A));
			w_file->FputUint8(char(0xFF));
			csum = 0xff;
			lp1 = 1;
			sub_state++;
			break;
		}
		case 4:
		{
			if(lp1 > 255)
			{
				sub_state = 6;
				break;
			}
			sub_state++;
			receive1byte();
			break;
		}
		case 5:
		{
			r_data = result_value;
			w_file->FputUint8(r_data);
			csum = csum + r_data;
			lp1++;
			sub_state = 4;
			break;
		}
		case 6:
		{
			// CHECK SUM�v�Z�A��������
			csum = 0 - csum;
			w_file->FputUint8(csum);
			w_len1--;
			sub_state = 3;
			break;
		}
		case 100:
		{
			// �[���u���b�N����
			if(w_len2 == 0)
			{
				sub_state = 200;
				break;
			}
			w_file->FputUint8(char(0x3A));
			w_file->FputUint8(w_len2);
			csum = w_len2;
			lp1 = 1;
			sub_state++;
			break;
		}
		case 101:
		{
			if(lp1 > w_len2)
			{
				sub_state = 103;
				break;
			}
			sub_state++;
			receive1byte();
			break;
		}
		case 102:
		{
			r_data = result_value;
			w_file->FputUint8(r_data);
			csum = csum + r_data;
			lp1++;
			sub_state = 101;
			break;
		}
		case 103:
		{
			// CHECK SUM�v�Z�A��������
			csum = 0 - csum;
			w_file->FputUint8(csum);
			sub_state = 200;
			break;
		}
		case 200:
		{
			w_file->FputUint8(char(0x3A));
			w_file->FputUint8(char(0x00));
			w_file->FputUint8(char(0x00));
			sub_state = 1000;
			break;
		}
		case 1000:
		{
			popstate();
			break;
		}
	}
}

void PC80SD::cmt_save_proc()
{
	switch(sub_state)
	{
		case 0:
		{
			sub_state++;
			send1byte(0x00);
			break;
		}
		case 1:
		{
			sub_state++;
			receivename(m_name);
			break;
		}
		case 2:
		{
			if(m_name[0] != 0x00)
			{
				addcmt(m_name, f_name);

				if(w_file)
				{
					w_file->Fclose();
				}

				// �t�@�C�������݂����delete
				if(SDFile::exists(f_name))
				{
					SDFile::remove(f_name);
				}

				// �t�@�C���I�[�v��
				w_file = new FILEIO();
				if (w_file->Fopen(create_sd_path(f_name), FILEIO_WRITE_BINARY))
				{
					sub_state++;
					send1byte(0x00);
					break;
				} else{
					sub_state = 1000;
					send1byte(0xf0);
					break;
				}
			} else {
				sub_state = 1000;
				send1byte(0xf6);
				break;
			}
			break;
		}
		case 3:
		{
			sub_state++;
			w_body();
			break;
		}
		case 4:
		{
			w_file->Fclose();
			delete w_file;
			w_file = nullptr;
			sub_state = 1000;
			break;
		}
		case 100:
		{
			break;
		}

		case 1000:
		{
			loopend();
			break;
		}
	}
}

void PC80SD::cmt_load_proc()
{
	switch(sub_state)
	{
		case 0:
		{
			sub_state=123;
			send1byte(0x00);
			break;
		}
		case 123:
		{
			flg = false;
			sub_state=1;
			receivename(m_name);
			break;
		}
		case 1:
		{
			if(m_name[0] != 0x00)
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
						flg = true;
						sub_state++;
						send1byte(0x00);
					}
					else
					{
						sub_state++;
						send1byte(0xf0);
						flg = false;
					}
				}
				else
				{
					sub_state++;
					send1byte(0xf1);
					flg = false;
				}
			} else {
				sub_state = 100;
			}
			break;
		}
		case 2:
		{
			if(flg == false)
			{
				sub_state = 1000;
				break;
			}
			rdata = 0;

			// �w�b�_�[���o�Ă���܂œǂݔ�΂�
			while (rdata != 0x3a && rdata != 0xd3)
			{
				rdata = file->FgetUint8();
				r_count++;
			}
			sub_state++;
			send1byte(rdata);
			break;
		}
		case 3:
		{
			// �w�b�_�[��0x3a�Ȃ瑱�s�A�Ⴆ�΃G���[
			if (rdata == 0x3a)
			{
				// START ADDRESS HI�𑗐M
				s_adrs1 = file->FgetUint8();
				r_count++;
				sub_state++;
				send1byte(s_adrs1);
			} else {
				file->Fclose();
				delete file;
				file = nullptr;
				sub_state = 1000;
				break;
			}
			break;
		}
		case 4:
		{
			// START ADDRESS LO�𑗐M
			s_adrs2 = file->FgetUint8();
			r_count++;
			sub_state++;
			send1byte(s_adrs2);
			s_adrs = s_adrs1 * 256 + s_adrs2;
			break;
		}
		case 5:
		{
			// CHECK SUM�𑗐M
			rdata = file->FgetUint8();
			r_count++;
			sub_state++;
			send1byte(rdata);
			break;
		}
		case 6:
		{
			// HEADER�𑗐M
			rdata = file->FgetUint8();
			r_count++;
			sub_state++;
			send1byte(rdata);
			break;
		}
		case 7:
		{
			// �f�[�^���𑗐M
			b_length = file->FgetUint8();
			r_count++;
			sub_state++;
			send1byte(b_length);
			break;
		}
		case 8:
		{
			// �f�[�^����0x00�łȂ��ԃ��[�v����
			if (b_length == 0x00)
			{
				sub_state = 70;
				break;
			}
			lp1 = 0;
			sub_state++;
			break;
		}
		case 9:
		{
			if(lp1 > b_length)
			{
				sub_state = 10;
				break;
			}
			rdata = file->FgetUint8();
			r_count++;
			send1byte(rdata);
			lp1++;
			break;
		}
		case 10:
		{
			// CHECK SUM�𑗐M
			rdata = file->FgetUint8();
			r_count++;
			sub_state++;
			send1byte(rdata);
			break;
		}
		case 11:
		{
			// �f�[�^���𑗐M
			b_length = file->FgetUint8();
			r_count++;
			sub_state=8;
			send1byte(b_length);
			break;
		}
		case 70:
		{
			// �f�[�^����0x00���������̌㏈��
			// CHECK SUM�𑗐M
			rdata = file->FgetUint8();
			r_count++;
			sub_state++;
			send1byte(rdata);
			break;
		}
		case 71:
		{
			// �t�@�C���G���h�ɒB���Ă�����FILE CLOSE
			if (f_length == r_count)
			{
				file->Fclose();
				delete file;
				file = nullptr;
			}
			sub_state = 1000;
			break;
		}

		case 100:
		{
			// �t�@�C�����̎w�肪�Ȃ������ꍇ
			// �t�@�C���G���h�ɂȂ��Ă��Ȃ���
			if (f_length > r_count)
			{
				sub_state++;
				send1byte(0x00);
				flg = true;
			}
			else
			{
				sub_state++;
				send1byte(0xf1);
				flg = false;
			}
			break;
		}
		case 101:
		{
			sub_state = 2;
			break;
		}

		case 1000:
		{
			loopend();
			break;
		}
	}
}

void PC80SD::cmt_5f9e_proc()
{
	switch(sub_state)
	{
		case 0:
		{
			sub_state++;
			send1byte(0x00);
			break;
		}
		case 1:
		{
			if(file == nullptr)
			{
				// �t�@�C���������܂ōs���Ă��邩�J���Ă��Ȃ��ꍇ0xff��Ԃ�
				sub_state++;
				send1byte(0xff);
				break;
			}
			rdata = file->FgetUint8();
			r_count++;
			sub_state++;
			send1byte(rdata);
			break;
		}
		case 2:
		{
			// �t�@�C���G���h�܂ŒB���Ă����FILE CLOSE
			if (f_length == r_count && file != nullptr)
			{
				file->Fclose();
				delete file;
				file = nullptr;
			}
			sub_state = 1000;
			break;
		}
		case 1000:
		{
			loopend();
			break;
		}
	}
}

void PC80SD::bas_load_proc()
{
	switch(sub_state)
	{
		case 0:
		{
			sub_state++;
			send1byte(0x00);
			break;
		}
		case 1:
		{
			flg = false;
			sub_state++;
			receivename(m_name);
			break;
		}
		case 2:
		{
			if(m_name[0] != 0x00)
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
						flg = true;
						sub_state++;
						send1byte(0x00);
					}
					else
					{
						sub_state++;
						send1byte(0xf0);
						flg = false;
					}
				}
				else
				{
					sub_state++;
					send1byte(0xf1);
					flg = false;
				}
			} else {
				sub_state = 100;
			}
			break;
		}
		case 3:
		{
			if(flg == false)
			{
				sub_state = 1000;
				break;
			}
			rdata = 0;

			// �w�b�_�[���o�Ă���܂œǂݔ�΂�
			while (rdata != 0x3a && rdata != 0xd3)
			{
				rdata = file->FgetUint8();
				r_count++;
			}
			sub_state++;
			// �w�b�_�[���M
			send1byte(rdata);
			break;
		}
		case 4:
		{
			// �w�b�_�[��0xd3�Ȃ瑱�s�A�Ⴆ�΃G���[
			if (rdata == 0xd3)
			{
				while (rdata == 0xd3)
				{
					rdata = file->FgetUint8();
					r_count++;
				}

				// ���f�[�^���M
				zcnt = 0;
				zdata = 1;

				sub_state++;
				send1byte(rdata);
			} else {
				file->Fclose();
				delete file;
				file = nullptr;
				sub_state = 1000;
				break;
			}
			break;
		}
		case 5:
		{
			// 0x00��11�����܂œǂݍ��݁A���M
			if(zcnt >= 11)
			{
				sub_state = 50;
				break;
			}

			rdata = file->FgetUint8();
			r_count++;
			sub_state++;
			send1byte(rdata);
			break;
		}
		case 6:
		{
			if (rdata == 0x00)
			{
				zcnt++;
				if (zdata != 0)
				{
					zcnt = 0;
				}
			}
			zdata = rdata;
			sub_state = 5;
			break;
		}
		case 50:
		{
			// �t�@�C���G���h�ɒB���Ă�����FILE CLOSE
			if (f_length == r_count)
			{
				file->Fclose();
				delete file;
				file = nullptr;
			}
			sub_state = 1000;
			break;
		}

		case 100:
		{
			// �t�@�C�����̎w�肪�Ȃ�������V��
			// �t�@�C���G���h�ɂȂ��Ă��Ȃ���
			if (f_length > r_count)
			{
				sub_state=3;
				send1byte(0x00);
				flg = true;
			}
			else
			{
				sub_state=3;
				send1byte(0xf1);
				flg = false;
			}
			break;
		}

		case 1000:
		{
			loopend();
			break;
		}
	}
}

void PC80SD::bas_save_proc()
{
	switch(sub_state)
	{
		case 0:
		{
			sub_state++;
			send1byte(0x00);
			break;
		}
		case 1:
		{
			flg = false;
			sub_state++;
			receivename(m_name);
			break;
		}
		case 2:
		{
			if(m_name[0] != 0x00)
			{
				addcmt(m_name, f_name);

				if(w_file)
				{
					w_file->Fclose();
				}

				// �t�@�C�������݂����delete
				if(SDFile::exists(f_name))
				{
					SDFile::remove(f_name);
				}

				// �t�@�C���I�[�v��
				w_file = new FILEIO();
				if (w_file->Fopen(create_sd_path(f_name), FILEIO_WRITE_BINARY))
				{
					sub_state++;
					send1byte(0x00);
					break;
				} else{
					sub_state = 1000;
					send1byte(0xf0);
					break;
				}
			} else {
				sub_state = 100;
			}
			break;
		}
		case 3:
		{
			sub_state++;
			// �X�^�[�g�A�h���X�擾
			receivebytes(2);
			break;
		}
		case 4:
		{
			s_adrs1 = receive_datas[0];
			s_adrs2 = receive_datas[1];
			// �X�^�[�g�A�h���X�Z�o
			s_adrs = s_adrs1 + s_adrs2 * 256;

			sub_state++;
			// �G���h�A�h���X�擾
			receivebytes(2);
			break;
		}
		case 5:
		{
			s_adrs1 = receive_datas[0];
			s_adrs2 = receive_datas[1];
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
			lp1 = s_adrs;
			sub_state++;
			break;
		}
		case 6:
		{
			if(lp1 > e_adrs)
			{
				sub_state = 8;
				break;
			}
			sub_state++;
			receive1byte();
			break;
		}
		case 7:
		{
			w_file->FputUint8(result_value);
			lp1++;
			sub_state = 6;
			break;
		}
		case 8:
		{
			// �I�� 0x00 x 9�񏑂�����
			for (lp1 = 1; lp1 <= 9; lp1++)
			{
				w_file->FputUint8(char(0x00));
			}
			w_file->Fclose();
			delete w_file;
			w_file = nullptr;
			sub_state = 1000;
			break;
		}
		case 100:
		{
			sub_state = 1000;
			send1byte(0xf1);
			break;
		}

		case 1000:
		{
			loopend();
			break;
		}
	}
}

void PC80SD::bas_kill_proc()
{
	switch(sub_state)
	{
		case 0:
		{
			sub_state++;
			send1byte(0x00);
			break;
		}
		case 1:
		{
			flg = false;
			sub_state++;
			receivename(m_name);
			break;
		}
		case 2:
		{
			if(m_name[0] != 0x00)
			{
				addcmt(m_name, f_name);

				// ��ԃR�[�h���M(OK)
				sub_state++;
				send1byte(0x00);
			} else {
				sub_state = 100;
			}
			break;
		}
		case 3:
		{
			// �t�@�C�������݂����delete
			if(SDFile::exists(f_name))
			{
				SDFile::remove(f_name);
				sub_state = 1000;
				send1byte(0x00);
				break;
			} else {
				sub_state = 1000;
				send1byte(0xf1);
				break;
			}
		}
		case 100:
		{
			sub_state = 1000;
			send1byte(0xf1);
			break;
		}
		case 1000:
		{
			loopend();
			break;
		}
	}
}

void PC80SD::sd_ropen_proc()
{
	switch(sub_state)
	{
		case 0:
		{
			sub_state++;
			send1byte(0x00);
			break;
		}
		case 1:
		{
			flg = false;
			sub_state++;
			receivename(f_name);
			break;
		}
		case 2:
		{
			// �t�@�C�������݂��Ȃ����ERROR
			if (SDFile::exists(f_name) == true)
			{
				file = new FILEIO();
				if (file->Fopen(create_sd_path(f_name), FILEIO_READ_BINARY))
				{
					// f_length�ݒ�Ar_count������
					f_length = file->FileLength();
					r_count = 0;
					// ��ԃR�[�h���M(OK)
					sub_state=1000;
					send1byte(0x00);
					break;
				} else {
					sub_state=1000;
					send1byte(0xf0);
					break;
				}
			} else {
				sub_state=1000;
				send1byte(0xf1);
				break;
			}
		}
		case 1000:
		{
			loopend();
			break;
		}
	}
}

void PC80SD::sd_waopen_proc()
{
	switch(sub_state)
	{
		case 0:
		{
			sub_state++;
			send1byte(0x00);
			break;
		}
		case 1:
		{
			flg = false;
			sub_state++;
			receivename(m_name);
			break;
		}
		case 2:
		{
			// �t�@�C���I�[�v��
			if (w_file != nullptr)
			{
				w_file->Fclose();
			}

			w_file = new FILEIO();
			if (w_file->Fopen(create_sd_path(w_name), FILEIO_WRITE_BINARY))
			{
				// Append�Ȃ̂ł���ł����̂��ȁc�c�H
				w_file->Fseek(0, FILEIO_SEEK_END);

				sub_state = 1000;
				// ��ԃR�[�h���M(OK)
				send1byte(0x00);
			}
			else
			{
				sub_state = 1000;
				send1byte(0xf0);
			}
			break;
		}
		case 1000:
		{
			loopend();
			break;
		}
	}
}


void PC80SD::sd_w1byte_proc()
{
	switch(sub_state)
	{
		case 0:
		{
			sub_state++;
			send1byte(0x00);
			break;
		}
		case 1:
		{
			sub_state++;
			receive1byte();
			break;
		}
		case 2:
		{
			int rdata = result_value;
			if (w_file != nullptr)
			{
				w_file->FputUint8(rdata);
				sub_state = 1000;
				// ��ԃR�[�h���M(OK)
				send1byte(0x00);
			}
			else
			{
				sub_state = 1000;
				send1byte(0xf0);
			}
			break;
		}
		case 1000:
		{
			loopend();
			break;
		}
	}
}

void PC80SD::sd_wnopen_proc()
{
	switch(sub_state)
	{
		case 0:
		{
			sub_state++;
			send1byte(0x00);
			break;
		}
		case 1:
		{
			flg = false;
			sub_state++;
			receivename(m_name);
			break;
		}
		case 2:
		{
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
				sub_state = 1000;
				// ��ԃR�[�h���M(OK)
				send1byte(0x00);
			}
			else
			{
				sub_state = 1000;
				send1byte(0xf0);
			}
			break;
		}
		case 1000:
		{
			loopend();
			break;
		}
	}
}

// 5ED9H���
void PC80SD::sd_wdirect_proc()
{
	switch(sub_state)
	{
		case 0:
		{
			sub_state++;
			send1byte(0x00);
			break;
		}
		case 1:
		{
			if (w_file != nullptr)
			{
				sub_state++;
				w_body();
			}
			else
			{
				sub_state = 1000;
				send1byte(0xf0);
				break;
			}
			break;
		}
		case 2:
		{
			w_file->Fclose();
			delete w_file;
			w_file = nullptr;
			// ��ԃR�[�h���M(OK)
			sub_state = 1000;
			send1byte(0x00);
			break;
		}
		case 1000:
		{
			loopend();
			break;
		}
	}
}

// write file close
void PC80SD::sd_wclose_proc()
{
	switch(sub_state)
	{
		case 0:
		{
			sub_state++;
			send1byte(0x00);
			break;
		}
		case 1:
		{
			w_file->Fclose();
			delete w_file;
			w_file = nullptr;
			sub_state = 1000;
			break;
		}
		case 1000:
		{
			loopend();
			break;
		}
	}
}

void PC80SD::dirlist_proc()
{
	switch(sub_state)
	{
		case 0:
		{
			sub_state++;
			send1byte(0);
			break;
		}
		case 1:
		{
			// 32+1��������M
			sub_state++;
			receivebytes(33);

			break;
		}
		case 2:
		{
			// OK
			memcpy(c_name, receive_datas, 33);
			sub_state++;
			bool result = sd_file.open(create_local_path(sd_folder_name));
			if (result)
			{
				// OK
				send1byte(0);
			} else {
				// error
				send1byte(0xf1);
			}
			break; 
		}
		case 3:
		{
			sd_entry = sd_file.openNextFile();
			cntl2 = 0;
			br_chk = 0;
			page = 1;
			sub_state++;
		}
		case 4:
		{
			if(br_chk != 0)
			{
				sub_state = 10;
			} else if (sd_entry.isValid())
			{
				sd_entry.getName(f_name, 36);
				unsigned int lp1 = 0;
				// �ꌏ���M
				// ��r������Ńt�@�C���l�[����/re�擪10�����܂Ŕ�r���Ĉ�v������̂������o��
				if (f_match(f_name, c_name))
				{
					// �ő�36���� or 0�ɂȂ�܂ő��M
					int len = 36;
					int slen = strlen(f_name);
					len = min(len, slen);
					sub_state++;
					sendbytes((uint8_t*)f_name, len);
				}
				else {
					sub_state = 6;
				}
			}
			break;
		}
		case 5:
		{
			sub_state++;
			sendbytes(cr_data, 2);
			cntl2++;
			break;
		}
		case 6:
		{
			if (!sd_entry.isValid() || cntl2 > 15)
			{
				// �p���E�ł��؂�I���w���v��
				sub_state++;
				send1byte(0xfe);
			} else {
				sub_state = 9;
			}
			break;
		}
		case 7:
		{
			sub_state++;
			receive1byte();
			break;
		}
		case 8:
		{
			br_chk = result_value;
			// �O�y�[�W����
			if (br_chk == 0x42)
			{
				// �擪�t�@�C����
				sd_file.rewindDirectory();
				// entry�l�X�V
				sd_entry = sd_file.openNextFile();
				// ������x�擪�t�@�C����
				sd_file.rewindDirectory();
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
						sd_entry = sd_file.openNextFile();
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
		case 9:
		{
			// �t�@�C�����܂�����Ȃ玟�ǂݍ��݁A�Ȃ���Αł��؂�w��
			if (sd_entry.isValid())
			{
				sd_entry = sd_file.openNextFile();
			}
			else
			{
				br_chk = 1;
			}
			// FDL�̌��ʂ�18�������Ȃ�p���w���v�������ɂ��̂܂܏I��
			if (!sd_entry.isValid() && cntl2 < 16 && page == 1)
			{
				sub_state++;
			} else {
				sub_state = 4;
			}
			break;
		}
		case 10:
		{
			sub_state++;
			sendbytes(end_data, 2);
			break;
		}
		case 11:
		{
			loopend();
			break;
		}
	}
}

void PC80SD::proc_state()
{

	switch(state)
	{
		case PC80SD_INITIALIZE:
		{
			switch (sub_state)
			{
			case 0:
			{
				write_signals(&port[1].outputs, 0);
				sub_state++;
				wait_count = 0;
				// break;
			}
			case 1:
			{
				wait_count++;
				if (wait_count > 1000)
				{
					sub_state++;
				}
				break;
			}
			case 2:
			{
				write_signals(&port[2].outputs, 0x00);
				state = PC80SD_WAIT_COMMAND;
				sub_state = 0;
			}
			}
			break;
		}
		case PC80SD_WAIT_COMMAND:
		{
			switch(sub_state)
			{
				case 0:
					sub_state++;
					receive1byte();
					break;
				case 1:
				{
					state = result_value;
					sub_state = 0;
					break;
				}
			}
			break;
		}
		case PC80SD_CMT_SAVE:
		{
			cmt_save_proc();
			break;
		}
		case PC80SD_CMT_LOAD:
		{
			cmt_load_proc();
			break;
		}
		case PC80SD_CMT_5F9E:
		{
			cmt_5f9e_proc();
			break;
		}
		case PC80SD_BAS_LOAD:
		{
			bas_load_proc();
			break;
		}
		case PC80SD_BAS_SAVE:
		{
			bas_save_proc();
			break;
		}
		case PC80SD_BAS_KILL:
		{
			bas_kill_proc();
			break;
		}
		case PC80SD_SD_ROPEN:
		{
			sd_ropen_proc();
			break;
		}
		case PC80SD_SD_WAOPEN:
		{
			sd_waopen_proc();
			break;
		}
		case PC80SD_SD_W1BYTE:
		{
			sd_w1byte_proc();
			break;
		}
		case PC80SD_SD_WNOPEN:
		{
			sd_wnopen_proc();
			break;
		}
		case PC80SD_SD_WDIRECT:
		{
			sd_wdirect_proc();
			break;
		}
		case PC80SD_SD_WCLOSE:
		{
			sd_wclose_proc();
			break;
		}
		case PC80SD_DIRLIST:
		{
			dirlist_proc();
			break;
		}
		case PC80SD_RCV1BYTE:
		{
			switch(sub_state)
			{
				case 0:
				{
					result_value = 0;
					sub_state++;
					receive4bit();
					break;
				}
				case 1:
				{
					result_value *= 16;
					sub_state++;
					receive4bit();
					break;
				}
				case 2:
				{
					popstate();
					break;
				}
			}
			break;
		}
		case PC80SD_RCV4BIT:
		{
			switch(sub_state)
			{
				case 0:
				{
					if((port[2].reg & 0x4) == 0)
					{
						return;
					}
					sub_state = 1;
				}
				case 1:
				{
					result_value += port[0].reg & 0xf;
					sub_state = 2;
					break;
				}
				case 2:
				{
					 write_signals(&port[2].outputs, 0x80 );
					sub_state = 3;
					break;
				}
				case 3:
				{
					if((port[2].reg & 0x4) != 0)
					{
						return;
					}
					sub_state = 4;
					break;
				}
				case 4:
				{
					write_signals(&port[2].outputs, 0x00);
					popstate();
//					wait_count = 0;
//					sub_state = 5;
					break;
				}
//				case 5:
//				{
//					wait_count++;
//					if(wait_count > 1000)
//					{
//						popstate();
//					}
//					break;
//				}
			}
			break;
			case PC80SD_SND1BYTE:
			{
				switch(sub_state)
				{
					case 0:
					{
						write_signals(&port[1].outputs, send_value);
						wait_count = 0;
						sub_state++;
						break;
					}
					case 1:
					{
						wait_count++;
						if (wait_count > 700)
						{
							sub_state++;
						}
						break;
					}
					case 2:
					{
						write_signals(&port[2].outputs, 0x80 );
						sub_state++;
						break;
					}
					case 3:
					{
						if((port[2].reg & 0x4) == 0)
						{
							return;
						}

						write_signals(&port[2].outputs, 0x00);
						sub_state++;
						break;
					}
					case 4:
					{
						if((port[2].reg & 0x4) != 0)
						{
							return;
						}
						popstate();
						break;
					}
				}
				break;
			}
			case PC80SD_RCVBYTES:
			{
				if(receive_datas_count == 0)
				{
					popstate();
					return;
				}
				if(sub_state == 0)
				{
					sub_state++;
					receive1byte();
				}
				else
				{
					receive_datas[receive_datas_index] = result_value;
					receive_datas_index++;
					receive_datas_count--;
					sub_state = 0;
				}
				break;
			}
			case PC80SD_SNDBYTES:
			{
				if(send_datas_count == 0)
				{
					popstate();
					return;
				}
				if(sub_state == 0)
				{
					sub_state++;
					send1byte(send_datas[send_datas_index]);
				}
				else
				{
					send_datas_index++;
					send_datas_count--;
					sub_state = 0;
				}
				break;
			}
			case PC80SD_RCVNAME:
			{
				switch(sub_state)
				{
					case 0:
					{
						lp2 = 0;
						sub_state++;
					}
					case 1:
					{
						sub_state++;
						receive1byte();
						break;
					}
					case 2:
					{
						char r_data = result_value;
						if(r_data != 0x22)
						{
							target_data[lp2] = r_data;
							lp2++;
						}
						if(lp2 == 33)
						{
							popstate();
							break;
						} else {
							sub_state = 1;
						}
						break;
					}
				}
				break;
			}
			case PC80SD_WBODY:
			{
				w_body_proc();
				break;
			}
		}
	}
}

void PC80SD::event_callback(int event_id, int error)
{
	switch (event_id)
	{
	case EVENT_TIMER:
		proc_state();
		break;
	}
}