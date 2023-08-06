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

// 保存フォルダ
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

	// 上記dir_pathにpathを連結してフルパスを作る
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
			L"."とL".."はスキップ
		*/
		if (L'.' == wpFileName[0])
		{
			if ((L'\0' == wpFileName[1]) || (L'.' == wpFileName[1] && L'\0' == wpFileName[2]))
			{
				continue;
			}
		}
		// フォルダはスキップ
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			continue;
		}

		currentEntry = SDFileEntry(ffd.cFileName);
		updated = true;
		break;
	}

	// currentEntryを更新出来なかった場合、ここで終了となる
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
				// ファイル名を取得
				currentEntry = SDFileEntry(ent->d_name);
				updated = true;
				break;
			}	
		}
	}

	// currentEntryを更新出来なかった場合、ここで終了となる
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
			  L"."とL".."はスキップ
		  */
		if (L'.' == wpFileName[0])
		{
			if ((L'\0' == wpFileName[1]) || (L'.' == wpFileName[1] && L'\0' == wpFileName[2]))
			{
				continue;
			}
		}
		// 一つ目のEntryを作っておく
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
				// ファイル名を取得
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

// 小文字->大文字
char upper(char c)
{
	if ('a' <= c && c <= 'z')
	{
		c = c - ('a' - 'A');
	}
	return c;
}

// f_nameとc_nameをc_nameに0x00が出るまで比較
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
//	// Windowsの場合
//	Sleep(milliseconds); // 引数はミリ秒
//#else
//	// Linux/Macの場合
//	usleep(microseconds); // 引数はマイクロ秒
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
	// HIGHになるまでループ
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

	// 受信
	uint8_t j_data = current_pc80sd->port[0].reg & 0xf;

	printf("rcv4bit: low wait\n");
	// LOWになるまでループ
	if((current_pc80sd->port[2].reg & 0x4))
	{
		current_check = CHECK_LOW;
	}
	// FLGをセット
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

	// FLGをリセット
	current_pc80sd->write_signals_thread(2, 0x00);

	
	// 時間計測のために現在の時間を保存
	struct timespec start_time, end_time;
	clock_gettime(CLOCK_MONOTONIC, &start_time);

	// 直後にrcv1byteした場合↑のLOWのチェックがZ80側でされない事があるのでそれなりに待ってやる
	printf("receive end wait start\n");
	thread_sleep(16000);
	printf("receive end wait end\n");

	clock_gettime(CLOCK_MONOTONIC, &end_time);
	double elapsed = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_nsec - start_time.tv_nsec) * 1e-9;
	
	printf("Elapsed time: %.9f seconds\n", elapsed);

	//printf("rcv4bit end\n");
	return (j_data);
}

// 1BYTE送信
void snd1byte(uint8_t i_data)
{
	//printf("snd1byte\n");
	if (current_pc80sd->request_terminate)
	{
		return;
	}
	// 下位ビットから8ビット分をセット
	current_pc80sd->write_signals_thread(1, i_data);

	// HIGHになるまでループ
	if(!(current_pc80sd->port[2].reg & 0x4))
	{
		current_check = CHECK_HIGH;
	}
	// FLGPINをHIGHに
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

	// LOWになるまでループ
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

// SD-CARDのFILELIST
void dirlist()
{
	printf("dirlist\n");
	// 比較文字列取得 32+1文字まで
	for (unsigned int lp1 = 0; lp1 <= 32; lp1++)
	{
		c_name[lp1] = rcv1byte();
	}
	printf("dirlist 2\n");

	SDFile file;
	bool result = file.open(create_local_path(sd_folder_name));
	if (result)
	{
		// 状態コード送信(OK)
		printf("send ok\n");
		snd1byte(0x00);
		printf("send ok 2\n");

		SDFileEntry entry = file.openNextFile();
		int cntl2 = 0;
		unsigned int br_chk = 0;
		int page = 1;
		// 全件出力の場合には16件出力したところで一時停止、キー入力により継続、打ち切りを選択
		while (br_chk == 0)
		{
			if (entry.isValid())
			{
				entry.getName(f_name, 36);
				unsigned int lp1 = 0;
				// 一件送信
				// 比較文字列でファイルネームを/re先頭10文字まで比較して一致するものだけを出力
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
				// 継続・打ち切り選択指示要求
				snd1byte(0xfe);

				// 選択指示受信(0:継続 B:前ページ 以外:打ち切り)
				br_chk = rcv1byte();
				// 前ページ処理
				if (br_chk == 0x42)
				{
					// 先頭ファイルへ
					file.rewindDirectory();
					// entry値更新
					entry = file.openNextFile();
					// もう一度先頭ファイルへ
					file.rewindDirectory();
					if (page <= 2)
					{
						// 現在ページが1ページ又は2ページなら1ページ目に戻る処理
						page = 0;
					}
					else
					{
						// 現在ページが3ページ以降なら前々ページまでのファイルを読み飛ばす
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
			// ファイルがまだあるなら次読み込み、なければ打ち切り指示
			if (entry.isValid())
			{
				entry = file.openNextFile();
			}
			else
			{
				br_chk = 1;
			}
			// FDLの結果が18件未満なら継続指示要求せずにそのまま終了
			if (!entry.isValid() && cntl2 < 16 && page == 1)
			{
				break;
			}
		}
		// 処理終了指示
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

// 比較文字列取得 32+1文字まで取得、ただしダブルコーテーションは無視する
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

// MONITOR Lコマンド .CMT LOAD
void cmt_load()
{
	bool flg = false;
	// DOSファイル名取得
	receive_name(m_name);
	// ファイル名の指定があるか
	if (m_name[0] != 0x00)
	{
		addcmt(m_name, f_name);

		// 指定があった場合
		// ファイルが存在しなければERROR
		if (SDFile::exists(f_name) == true)
		{
			// ファイルオープン
			file = new FILEIO();
			if (file->Fopen(create_local_path("%s/%s", sd_folder_name,  f_name), FILEIO_READ_BINARY))
			{
				// f_length設定、r_count初期化
				f_length = file->FileLength();
				r_count = 0;
				// 状態コード送信(OK)
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
		// ファイル名の指定がなかった場合
		// ファイルエンドになっていないか
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

	// 良ければファイルエンドまで読み込みを続行する
	if (flg == true)
	{
		int rdata = 0;

		// ヘッダーが出てくるまで読み飛ばし
		while (rdata != 0x3a && rdata != 0xd3)
		{
			rdata = file->FgetUint8();
			r_count++;
		}
		// ヘッダー送信
		snd1byte(rdata);

		// ヘッダーが0x3aなら続行、違えばエラー
		if (rdata == 0x3a)
		{
			// START ADDRESS HIを送信
			s_adrs1 = file->FgetUint8();
			r_count++;
			snd1byte(s_adrs1);

			// START ADDRESS LOを送信
			s_adrs2 = file->FgetUint8();
			r_count++;
			snd1byte(s_adrs2);
			s_adrs = s_adrs1 * 256 + s_adrs2;

			// CHECK SUMを送信
			rdata = file->FgetUint8();
			r_count++;
			snd1byte(rdata);

			// HEADERを送信
			rdata = file->FgetUint8();
			r_count++;
			snd1byte(rdata);

			// データ長を送信
			b_length = file->FgetUint8();
			r_count++;
			snd1byte(b_length);

			// データ長が0x00でない間ループする
			while (b_length != 0x00)
			{
				for (unsigned int lp1 = 0; lp1 <= b_length; lp1++)
				{
					// 実データを読み込んで送信
					rdata = file->FgetUint8();
					r_count++;
					snd1byte(rdata);
				}
				// CHECK SUMを送信
				rdata = file->FgetUint8();
				r_count++;
				snd1byte(rdata);
				// データ長を送信
				b_length = file->FgetUint8();
				r_count++;
				snd1byte(b_length);
			}
			// データ長が0x00だった時の後処理
			// CHECK SUMを送信
			rdata = file->FgetUint8();
			r_count++;
			snd1byte(rdata);

			// ファイルエンドに達していたらFILE CLOSE
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

// 5F9EH CMT 1Byte読み込み処理の代替、OPENしているファイルの続きから1Byteを読み込み、送信
void cmt_5f9e(void)
{
	if (file == nullptr)
	{
		// 開いてない
		return;
	}
	int rdata = file->FgetUint8();
	r_count++;
	snd1byte(rdata);
	// ファイルエンドまで達していればFILE CLOSE
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
	// ヘッダー 0x3A書き込み
	w_file->FputUint8(char(0x3A));
	// スタートアドレス取得、書き込み
	s_adrs1 = rcv1byte();
	s_adrs2 = rcv1byte();
	w_file->FputUint8(s_adrs2);
	w_file->FputUint8(s_adrs1);
	// CHECK SUM計算、書き込み
	csum = 0 - (s_adrs1 + s_adrs2);
	w_file->FputUint8(csum);
	// スタートアドレス算出
	s_adrs = s_adrs1 + s_adrs2 * 256;
	// エンドアドレス取得
	s_adrs1 = rcv1byte();
	s_adrs2 = rcv1byte();
	// エンドアドレス算出
	e_adrs = s_adrs1 + s_adrs2 * 256;
	// ファイル長算出、ブロック数算出
	w_length = e_adrs - s_adrs + 1;
	w_len1 = w_length / 255;
	w_len2 = w_length % 255;

	// 実データ受信、書き込み
	// 0xFFブロック
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
		// CHECK SUM計算、書き込み
		csum = 0 - csum;
		w_file->FputUint8(csum);
		w_len1--;
	}

	// 端数ブロック処理
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
		// CHECK SUM計算、書き込み
		csum = 0 - csum;
		w_file->FputUint8(csum);
	}
	w_file->FputUint8(char(0x3A));
	w_file->FputUint8(char(0x00));
	w_file->FputUint8(char(0x00));
}

// MONITOR Wコマンド .CMT SAVE
void cmt_save(void)
{
	uint8_t r_data, csum;
	// DOSファイル名取得
	receive_name(m_name);
	// ファイル名の指定が無ければエラー
	if (m_name[0] != 0x00)
	{
		addcmt(m_name, f_name);

		if (w_file != nullptr)
		{
			w_file->Fclose();
		}
		// ファイルが存在すればdelete
		if (SDFile::exists(f_name) == true)
		{
			SDFile::remove(f_name);
		}
		// ファイルオープン
		w_file = new FILEIO();
		//  SD.open( f_name, FILE_WRITE );
		if (w_file->Fopen(create_sd_path(f_name), FILEIO_WRITE_BINARY))
		{
			// 状態コード送信(OK)
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

// BASICプログラムのSAVE処理
void bas_save(void)
{
	unsigned int lp1;

	// DOSファイル名取得
	receive_name(m_name);
	// ファイル名の指定が無ければエラー
	if (m_name[0] != 0x00)
	{
		addcmt(m_name, f_name);

		if (w_file)
		{
			w_file->Fclose();
		}
		// ファイルが存在すればdelete
		if (SDFile::exists(f_name) == true)
		{
			SDFile::remove(f_name);
		}
		// ファイルオープン
		//  w_file = SD.open( f_name, FILE_WRITE );
		w_file = new FILEIO();
		if (w_file->Fopen(create_sd_path(f_name), FILEIO_WRITE_BINARY))
		{
			// 状態コード送信(OK)
			snd1byte(0x00);

			// スタートアドレス取得
			s_adrs1 = rcv1byte();
			s_adrs2 = rcv1byte();
			// スタートアドレス算出
			s_adrs = s_adrs1 + s_adrs2 * 256;
			// エンドアドレス取得
			s_adrs1 = rcv1byte();
			s_adrs2 = rcv1byte();
			// エンドアドレス算出
			e_adrs = s_adrs1 + s_adrs2 * 256;
			// ヘッダー 0xD3 x 9回書き込み
			for (lp1 = 0; lp1 <= 9; lp1++)
			{
				w_file->FputUint8(char(0xD3));
			}
			// DOSファイル名の先頭6文字をファイルネームとして書き込み
			for (lp1 = 0; lp1 <= 5; lp1++)
			{
				w_file->FputUint8(m_name[lp1]);
			}
			// 実データ (e_adrs - s_adrs +1)を受信、書き込み
			for (lp1 = s_adrs; lp1 <= e_adrs; lp1++)
			{
				w_file->FputUint8(rcv1byte());
			}
			// 終了 0x00 x 9回書き込み
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

// BASICプログラムのKILL処理
void bas_kill(void)
{
	unsigned int lp1;

	// DOSファイル名取得
	receive_name(m_name);
	// ファイル名の指定が無ければエラー
	if (m_name[0] != 0x00)
	{
		addcmt(m_name, f_name);

		// 状態コード送信(OK)
		snd1byte(0x00);
		// ファイルが存在すればdelete
		if (SDFile::exists(f_name) == true)
		{
			SDFile::remove(f_name);
			// 状態コード送信(OK)
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

// BASICプログラムのLOAD処理
void bas_load(void)
{
	bool flg = false;
	// DOSファイル名取得
	receive_name(m_name);
	// ファイル名の指定があるか
	if (m_name[0] != 0x00)
	{
		addcmt(m_name, f_name);

		// 指定があった場合
		// ファイルが存在しなければERROR
		if (SDFile::exists(f_name) == true)
		{
			// ファイルオープン
			file = new FILEIO();
			// file = filSD.open( f_name, FILE_READ );
			//  if( true == file ){
			if (file->Fopen(create_sd_path(f_name), FILEIO_READ_BINARY))
			{
				// f_length設定、r_count初期化
				f_length = file->FileLength();
				r_count = 0;
				// 状態コード送信(OK)
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
		// ファイル名の指定がなかった場合
		// ファイルエンドになっていないか
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
	// 良ければファイルエンドまで読み込みを続行する
	if (flg == true)
	{
		int rdata = 0;

		// ヘッダーが出てくるまで読み飛ばし
		while (rdata != 0x3a && rdata != 0xd3)
		{
			rdata = file->FgetUint8();
			r_count++;
		}
		// ヘッダー送信
		snd1byte(rdata);
		// ヘッダーが0xd3なら続行、違えばエラー
		if (rdata == 0xd3)
		{
			while (rdata == 0xd3)
			{
				rdata = file->FgetUint8();
				r_count++;
			}

			// 実データ送信
			int zcnt = 0;
			int zdata = 1;

			snd1byte(rdata);

			// 0x00が11個続くまで読み込み、送信
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

			// ファイルエンドに達していたらFILE CLOSE
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
	// ファイルが存在しなければERROR
	if (SDFile::exists(f_name) == true)
	{
		// ファイルオープン
		//  file = SD.open( f_name, FILE_READ );
		file = new FILEIO();
		if (file->Fopen(create_sd_path(f_name), FILEIO_READ_BINARY))
		{
			// f_length設定、r_count初期化
			f_length = file->FileLength();
			r_count = 0;
			// 状態コード送信(OK)
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
	// ファイルオープン
	if (w_file != nullptr)
	{
		w_file->Fclose();
	}
	// SD.open( w_name, FILE_WRITE );
	w_file = new FILEIO();
	if (w_file->Fopen(create_sd_path(w_name), FILEIO_WRITE_BINARY))
	{
		// 状態コード送信(OK)
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
	// ファイルが存在すればdelete
	if (SDFile::exists(w_name))
	{
		SDFile::remove(w_name);
	}
	// ファイルオープン
	w_file = new FILEIO();
	if (w_file->Fopen(create_sd_path(w_name), FILEIO_WRITE_BINARY))
	{
		// 状態コード送信(OK)
		snd1byte(0x00);
	}
	else
	{
		snd1byte(0xf0);
	}
}

// 5ED9H代替
void sd_wdirect(void)
{
	if (w_file != nullptr)
	{
		w_body();
		w_file->Fclose();
		delete w_file;
		w_file = nullptr;
		// 状態コード送信(OK)
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

// write 1byte 5F2FH代替
void sd_w1byte(void)
{
	int rdata = rcv1byte();
	if (w_file != nullptr)
	{
		w_file->FputUint8(rdata);
		// 状態コード送信(OK)
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
				// 70h:PC-8001 CMTファイル SAVE
			case 0x70:
				////    Serial.println("CMT SAVE START");
				// 状態コード送信(OK)
				snd1byte(0x00);
				cmt_save();
				break;
				// 71h:PC-8001 CMTファイル LOAD
			case 0x71:
				////    Serial.println("CMT LOAD START");
				// 状態コード送信(OK)
				snd1byte(0x00);
				cmt_load();
				break;
				// 72h:PC-8001 5F9EH READ ONE BYTE FROM CMT
			case 0x72:
				////    Serial.println("CMT_5F9E START");
				// 状態コード送信(OK)
				snd1byte(0x00);
				cmt_5f9e();
				break;
				// 73h:PC-8001 CMTファイル BASIC LOAD
			case 0x73:
				////    Serial.println("CMT BASIC LOAD START");
				// 状態コード送信(OK)
				snd1byte(0x00);
				bas_load();
				break;
				// 74h:PC-8001 CMTファイル BASIC SAVE
			case 0x74:
				////    Serial.println("CMT BASIC SAVE START");
				// 状態コード送信(OK)
				snd1byte(0x00);
				bas_save();
				break;
				// 75h:PC-8001 CMTファイル KILL
			case 0x75:
				////    Serial.println("CMT KILL START");
				// 状態コード送信(OK)
				snd1byte(0x00);
				bas_kill();
				break;
				// 76h:PC-8001 SD FILE READ OPEN
			case 0x76:
				////    Serial.println("SD FILE READ OPEN START");
				// 状態コード送信(OK)
				snd1byte(0x00);
				sd_ropen();
				break;
				// 77h:PC-8001 SD FILE WRITE APPEND OPEN
			case 0x77:
				////    Serial.println("SD FILE WRITE APPEND OPEN START");
				// 状態コード送信(OK)
				snd1byte(0x00);
				sd_waopen();
				break;
				// 78h:PC-8001 SD FILE WRITE 1Byte
			case 0x78:
				////    Serial.println("SD FILE WRITE 1Byte START");
				// 状態コード送信(OK)
				snd1byte(0x00);
				sd_w1byte();
				break;
				// 79h:PC-8001 SD FILE WRITE NEW OPEN
			case 0x79:
				////    Serial.println("SD FILE WRITE NEW OPEN START");
				// 状態コード送信(OK)
				snd1byte(0x00);
				sd_wnopen();
				break;
				// 7Ah:PC-8001 SD WRITE 5ED9H
			case 0x7A:
				////    Serial.println("SD WRITE 5ED9H START");
				// 状態コード送信(OK)
				snd1byte(0x00);
				sd_wdirect();
				break;
				// 7Bh:PC-8001 SD FILE WRITE CLOSE
			case 0x7B:
				////    Serial.println("SD FILE WRITE CLOSE START");
				// 状態コード送信(OK)
				snd1byte(0x00);
				sd_wclose();
				break;

				// 83hでファイルリスト出力
			case 0x83:
				////    Serial.println("FILE LIST START");
				// 状態コード送信(OK)
				printf("dirlist ok send start\n");
				snd1byte(0x00);
				printf("dirlist ok send end\n");
				dirlist();
				break;
			default:
				// 状態コード送信(CMD ERROR)
				snd1byte(0xF4);
			}
		}
		else
		{
			// 状態コード送信(ERROR)
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

	// 時間はテキトー(1.6MHz)
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
	// Queueに蓄積されたシグナルをメインスレッドで処理する
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