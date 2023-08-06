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
#include <pthread.h>
#endif


class SignalNode {
public:
    int ch;
    uint32_t data;
    SignalNode* next;
};

class SignalQueue {
private:
    SignalNode* front;
    SignalNode* rear;

public:
    SignalQueue() : front(nullptr), rear(nullptr) {}

    ~SignalQueue() {
        while (front != nullptr) {
            SignalNode* temp = front;
            front = front->next;
            delete temp;
        }
    }

    void Enqueue(int ch, uint32_t data) {
        SignalNode* newNode = new SignalNode;
        newNode->ch = ch;
        newNode->data = data;
        newNode->next = nullptr;

        if (rear == nullptr) {
            front = rear = newNode;
        } else {
            rear->next = newNode;
            rear = newNode;
        }
    }

    void Dequeue() {
        if (front == nullptr)
            return; // Underflow case

        SignalNode* temp = front;
        front = front->next;

        if (front == nullptr)
            rear = nullptr;

        delete temp;
    }

    SignalNode* Front() {
        return front;
    }

    void Clear()
    {
	while (front != nullptr) {
	    SignalNode* temp = front;
	    front = front->next;
	    delete temp;
	}
	rear = nullptr;
    }

    bool IsEmpty() {
        return front == nullptr;
    }
};

class PC80SD : public DEVICE
{
private:
	void proc_signal();

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
	Stack stateStack;
public:
	SignalQueue signalQueue;
	int register_id;

	struct {
		uint8_t reg;
		// output signals
		outputs_t outputs;
	} port[3];

#if defined(_WIN32) || defined(_WIN64)
	unsigned threadID;
	uintptr_t threadHandle;
#else
	pthread_t threadID;
#endif
	bool request_terminate;
	int sleep_counter;

	PC80SD(VM_TEMPLATE* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu)
	{
		for(int i = 0; i < 3; i++) {
			initialize_output_signals(&port[i].outputs);
			port[i].reg = 0;//0xff;
		}
		request_terminate = false;
		sleep_counter = 0;

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

