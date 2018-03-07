//============================================================================
// Name        : CS4560.cpp
// Author      : Anthony Shay
// Description : Program 3, Write a program which implements a multiple producer - multiple consumer environment.
//============================================================================

#include <iostream>
#include <string>
#include <cstdlib>
#include <stdlib.h>
#include <pthread.h>
#include "buffer.h"
using namespace std;

string timeToSleep;
int numberProducer;
int numberConsumer;
void printBuffer();

typedef struct{
	int value;
	struct process *list;
} semaphore;

struct process{
	pthread_t pid;
	process * next;
};

//Semaphore functions
void wait(semaphore *S);
void signal(semaphore *S);
void block(semaphore *S);
void wakeup(semaphore *S);

//The Buffer
buffer_item buffer[BUFFER_SIZE];

//The Semaphores
static semaphore empty;
static semaphore full;
static semaphore mutex;
static int itemIn;
static int itemOut;

//Thread Functions
int insert_item(buffer_item item);
int remove_item(buffer_item *item);

//Threads
void* producer(void* param);
void* consumer(void* param);

int main(int argc, char *argv[]) {
	cout << "Main Thread Beginning\n";

	/* 1. Get command line arguments */
	timeToSleep = argv[1];
	numberProducer = stoi(argv[2]);
	numberConsumer = stoi(argv[3]);

	/* 2. Initialize buffer n stuff*/
	for(int x = 0; x < BUFFER_SIZE; ++x){
		buffer[x] = -1;
	}
	empty.value = BUFFER_SIZE;
	empty.list = nullptr;
	full.value = 0;
	full.list = nullptr;
	mutex.value = 1;
	mutex.list = nullptr;
	itemIn = 0;
	itemOut = 0;


	/* 3. Create producer threads. */
	pthread_t producers[numberProducer];
	for(int x = 0; x < numberProducer; ++x){
		if( pthread_create(&producers[x], nullptr, producer, (void*)"") == 0){
			cout << "Creating producer thread with id " << producers[x] << endl;
		}else{
			cout << "Producer creation error.\n";
		}
	}

	/* 4. Create consumer threads.  */
	pthread_t consumers[numberConsumer];
	for(int x = 0; x < numberConsumer; ++x){
		if( pthread_create(&consumers[x], nullptr, consumer, (void*)"") == 0){
				cout << "Creating consumer thread with id " << consumers[x] << endl;
			}else{
				cout << "Consumer creation error.\n";
			}
	}


	/* 5.  Sleep. */
	string toSleep = "sleep ";
	toSleep += timeToSleep;
	toSleep += "s";
	cout << "Main thread sleeping for " << timeToSleep << " seconds" << endl;
	system(toSleep.c_str());


	/* 6.  Kill threads and exit.  */
	cout << "\nMain thread exiting\n";
	return 0;
}

/*
 * inserts an item into the buffer
 * return 0 if successful, otherwise return -1 indicating an error condition
 */
int insert_item(buffer_item item){

	if(empty.value <= 0){
		return -1;
	}else{
		wait(&mutex);
		cout << "\nInsert_item inserted item " << item << " at position " << itemIn << endl;
		buffer[itemIn] = item;
		--empty.value;
		++full.value;
		itemIn = (itemIn + 1) % BUFFER_SIZE;
		printBuffer();
		signal(&mutex);
	}
	return 0;
}

/*
 * remove an object from buffer placing it in item
 * return 0 if successful, otherwise return -1 indicating an error condition
 */
int remove_item(buffer_item* item){

	if(empty.value >= BUFFER_SIZE){
		return -1;
	}else{
		wait(&mutex);
		cout << "\nRemove_item removed item " << buffer[itemOut] << " at position " << itemOut << endl;
		*item = buffer[itemOut];
		buffer[itemOut] = -1;
		++empty.value;
		--full.value;
		itemOut = (itemOut + 1) % BUFFER_SIZE;
		printBuffer();
		signal(&mutex);
	}
	return 0;
}

void printBuffer(){
	for(int x = 0; x < BUFFER_SIZE; ++x){
		cout << "[";
		if(buffer[x] == -1){
			cout << "empty";
		}else{
			cout << buffer[x];
		}
		cout << "]";
	}
	cout << "\tin = " << itemIn << "  out = " << itemOut << endl << endl;
}

void* producer(void* param){
	buffer_item item;

	while(true){
//		generate random number
		int randNumb = 1 + rand() % 3;

//		sleep for a random amount of time
		string toSleep = "sleep ";
		toSleep += to_string(randNumb);
		toSleep += "s";
		cout << "Producer thread " << pthread_self() << " sleeping for " << randNumb << " seconds\n";
		system(toSleep.c_str());

//		generate a random number(product) and put it in the buffer
		item = 1 + rand() % 100;

		if(insert_item(item) < 0){
			cout << "Producer error - buffer full" << endl;
		}else{
			cout << "Producer thread " << pthread_self() << " inserted value " << item << endl;
		}
	}
	return nullptr;
}

void* consumer(void* param){
	buffer_item item;

	while(true){
//		generate random number
		int randNumb = 1 + rand() % 3;

//		sleep for a random amount of time
		string toSleep = "sleep ";
		toSleep += to_string(randNumb);
		toSleep += "s";
		cout << "Consumer thread " << pthread_self() << " sleeping for " << randNumb << " seconds\n";
		system(toSleep.c_str());

//		remove a number (product) from the buffer and display it
		if(remove_item(&item) <0){
			cout << "Consumer error - buffer empty" << endl;
		}else{
			cout << "Consumer thread " << pthread_self() << " removed value " << item << endl;
		}

	}
	return nullptr;
}

void wait(semaphore *S){
	S->value--;
	if(S->value < 0){
		block(S);
	}
}

void signal(semaphore *S){
	S->value++;
	if(S->value <= 0){
		wakeup(S);
	}
}

void block(semaphore *S){
	process* P = new process();
	P->pid = pthread_self();
	P->next = nullptr;
	if(S->list == nullptr){
		S->list = P;
	}else{
		process *temp = S->list;
		while(temp->next != nullptr){
			temp = temp->next;
		}
		temp->next = P;
	}
	/*
	 * I was unable to find a workable solution in linux to reliably suspend threads without busy waiting.
	 * The code below is how to suspend a process, but it does not work on threads.
	 */
//	string suspend = "kill -TSTP " + to_string(P->pid);
//	system(suspend.c_str());
}

void wakeup(semaphore *S){
	process *temp = S->list;
	if(temp != nullptr){
		S->list = S->list->next;
		/*
		 * See explanation in block for 2 lines below
		 */
//		string unsuspend = "kill -CONT " + to_string(temp->pid);
//		system(unsuspend.c_str());
		delete temp;
	}
}
