/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2009.03.08-

	[ PC-8001(mk2)_SD ]
*/

#ifndef _PC80SD_H_
#define _PC80SD_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

#if !(defined(_WIN32) || defined(_WIN64))
#include <dirent.h>
#include <sys/stat.h>
#endif

#if !(defined(_WIN32) || defined(_WIN64))
#include <pthread.h>
#endif

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

class StackNode {
public:
    int data;
    StackNode* next;
};

class Stack {
private:
    StackNode* top_node;
public:
    Stack() : top_node(nullptr) {}

    ~Stack() {
        while (!empty()) {
            pop();
        }
    }

    void push(int value) {
        StackNode* new_node = new StackNode();
        new_node->data = value;
        new_node->next = top_node;
        top_node = new_node;
    }

    void pop() {
        if (empty()) {
            // std::cerr << "Stack is empty, cannot pop a value\n";
            return;
        }
        StackNode* temp = top_node;
        top_node = top_node->next;
        delete temp;
    }

    int top() const {
        if (empty()) {
            // std::cerr << "Stack is empty, cannot get top\n";
            return -1;
        }
        return top_node->data;
    }

    bool empty() const {
        return top_node == nullptr;
    }

    void clear() {
	while (!empty()) {
	    pop();
	}
    }
};

class PC80SD : public DEVICE
{
private:
	void proc_state();
	void proc_command();
	void sendbytes(const uint8_t*data, int sz);
	void receivebytes(int count);
	void receive1byte();
	void receive4bit();
	void popstate();
	void send1byte(uint8_t data);
	void loopend();
	void receivename(char *f_name);
	void w_body();
	void w_body_proc();

	void dirlist_proc();
	void cmt_save_proc();
	void cmt_load_proc();
	void cmt_5f9e_proc();
	void bas_load_proc();
	void bas_save_proc();
	void bas_kill_proc();
	void sd_ropen_proc();
	void sd_waopen_proc();
	void sd_w1byte_proc();
	void sd_wnopen_proc();
	void sd_wdirect_proc();
	void sd_wclose_proc();

	int state;
	int sub_state;
	int result_value;
	int send_value;
	int sd_command;
	uint8_t receive_datas[256];
	uint8_t send_datas[65536];
	int send_datas_count;
	int send_datas_index;
	int receive_datas_count;
	int receive_datas_index;
	int wait_count;
	char* target_data;
	Stack stateStack;
	SDFile sd_file;
	SDFileEntry sd_entry;
	int cntl2;
	unsigned int br_chk;
	int page;
	bool flg;
	unsigned int lp1;
	unsigned int lp2;
	int rdata;
	uint8_t r_data;
	uint8_t csum;
	int zcnt;
	int zdata;
public:
	int register_id;

	struct {
		uint8_t reg;
		// output signals
		outputs_t outputs;
	} port[3];

	PC80SD(VM_TEMPLATE* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu)
	{
		for(int i = 0; i < 3; i++) {
			initialize_output_signals(&port[i].outputs);
			port[i].reg = 0;//0xff;
		}
		state = sub_state = 0;

		set_device_name(_T("PC8001(mk2)_SD"));
	}
	~PC80SD();
	
	// common functions
	void initialize();
	void release();
	void reset();
	void write_signal(int id, uint32_t data, uint32_t mask);
	void event_callback(int event_id, int error);

	void write_signals_thread(int id, uint32_t data);

	void set_context_port_a(DEVICE* device, int id, uint32_t mask, int shift)
	{
		register_output_signal(&port[0].outputs, device, id, mask, shift);
	}
	void set_context_port_b(DEVICE* device, int id, uint32_t mask, int shift)
	{
		register_output_signal(&port[1].outputs, device, id, mask, shift);
	}
	void set_context_port_c(DEVICE* device, int id, uint32_t mask, int shift)
	{
		register_output_signal(&port[2].outputs, device, id, mask, shift);
	}
};

#endif

