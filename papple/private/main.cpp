#include <stdio.h>#include <iostream.h>#include <stdlib.h>#include <MacMemory.h>#include <MacTypes.h>#include <OpenTransport.h>#include <OpenTptInternet.h>#include <Threads.h>#define N_SLOTS 4void hexdump(char* ptr, unsigned int len) {	int i;	printf("%p: ", ptr);	for (i=0; i < len; i++) {		printf("%02x", (unsigned char) ptr[i]);		if (i % 16 == 15)			printf ("\n%p: ", &ptr[i+1]);		else if (i % 4 == 3)			printf(" ");		}	printf("\n");}void assertp(char* message, int b){	if (!b) printf("assert fail: %s\n", message);}InetAddress peer;ByteCount addrlen;EndpointRef endpoint = kOTInvalidEndpointRef;void send(char* buf, unsigned int length) {	int sent = OTSnd(endpoint, buf, length, 0);	assertp("OTSnd failed", send >= 0);}void send_str(char* buf) {	send(buf, strlen(buf));}int receive_into(char *buf, unsigned int length) {	OTFlags junkFlags;	OSStatus err;	OTResult lookResult;		int received = OTRcv(endpoint, (void*) buf, length, &junkFlags);		if (received > 0) {		return received;	}		err = received;		if (err == kOTLookErr)	{		lookResult = OTLook(endpoint);		switch (lookResult)		{			case T_DISCONNECT:				printf("client disconnected\n");				err = OTRcvDisconnect(endpoint, nil);				exit(0);				break;			case T_ORDREL:				err = OTRcvOrderlyDisconnect(endpoint);				printf("client disconnected orderly\n");				if (err == noErr)				{					err = OTSndOrderlyDisconnect(endpoint);				}				exit(0);				break;			default:				printf("other connection error type: %d\n", lookResult);				break;		}	}}unsigned receive_number() {	char recv_buffer[9] = { 0 };	char c;	int i = 0;		for (i = 0; i < 8; i++) {		receive_into(&c, 1);		if ((c < '0') || (c > '9')) {			//printf("done reading number.\n");			break;		}				//printf("got number %c\n", c);		recv_buffer[i] = c;	}	return atoi(recv_buffer);}Handle notes[N_SLOTS] = { NULL };struct note {	unsigned int size;	char* data;};void print_slots() {	int i;		for (i = 0; i < N_SLOTS; i++) {		printf("[%d]: ", i);				if (notes[i] == NULL) {			printf("empty\n");			continue;		}		printf("%p -> %p size=%d data=%p\n", notes[i], (void*) *notes[i],			((struct note*) *notes[i])->size,			(void*) ((struct note*) *notes[i])->data);	}	printf("********************************************************\n");}int FindFreeSlot() {	int i;		for (i=0; i < N_SLOTS; i++) {		if (notes[i] == NULL)			return i;	}		return -1;}Handle AllocNote(unsigned int size) {	Handle hNote;	Handle hData;	struct note* note;		hNote = NewHandle(sizeof(struct note));	note = (struct note*) *hNote;	note->size = size;	hData = NewHandle(size);	note->data = *hData;	//printf("reading %d bytes of data to %p\n", size, note->data);	send_str("Data: ");	receive_into(note->data, note->size);	return hNote;}void AddNote() {	int slot;	unsigned int size;		slot = FindFreeSlot();	if (slot == -1) {		send_str("No more space!\r");		return;	}		send_str("Size: ");	size = receive_number();		notes[slot] = AllocNote(size);	//printf("ID: %d\n", slot);}void DeleteNote() {	unsigned int id;	send_str("ID: ");	id = receive_number();		if ((id >= N_SLOTS) || (notes[id] == NULL)) {		printf("Invalid ID!\n");		return;	};	DisposeHandle(notes[id]);	notes[id] = NULL;}void PrintNote() {	unsigned int id;	struct note* note;	send_str("ID: ");	id = receive_number();	if ((id >= N_SLOTS) || (notes[id] == NULL)) {		send_str("Invalid ID!\n");		return;	};	note = (struct note*) *notes[id];	printf("note: %p size=%d data=%p\n", note, note->size, (void*) note->data);	send(note->data, note->size);}void EditNote() {	unsigned int id;	struct note* note;	send_str("ID: ");	id = receive_number();	if ((id >= N_SLOTS) || (notes[id] == NULL)) {		send_str("Invalid ID!\n");		return;	};	note = (struct note*) *notes[id];	send_str("Data: ");	receive_into(note->data, note->size);}char menu() {	char c[2];	print_slots();	send_str("[c]eate note\r"		"[r]ead note\r"		"[u]pdate note\r"		"[d]elete note\r"		"[e]xit\r"		"> ");	receive_into(c, 2);	return c[0];}static pascal void yield_notifier(void* contextPtr, OTEventCode code, OTResult result, void* cookie){	OSStatus err;	switch (code)	{		case kOTSyncIdleEvent:			err = YieldToAnyThread();			assertp("YieldToAnyThread failed", err == noErr);			break;		default:			break;	}}void main() {	OSStatus err;	InetHost host;	TCall sndCall;	err = InitOpenTransport();	if (err != noErr) {		printf("InitOpenTransport failed\n");		exit(1);	}	endpoint = OTOpenEndpoint(OTCreateConfiguration(kTCPName), 0, nil, &err);	if (err != noErr) {		printf("OTOpenEndpoint failed\n");		exit(1);	}	err = OTSetSynchronous(endpoint);	if (err != noErr) {		printf("OTSetSynchronous failed\n");		exit(1);	}	err = OTSetBlocking(endpoint);	if (err != noErr) {		printf("OTSetBlocking failed\n");		exit(1);	}	err = OTInstallNotifier(endpoint, yield_notifier, nil);	if (err != noErr) {		printf("OTInstallNotifier failed\n");		exit(1);	}	err = OTUseSyncIdleEvents(endpoint, true);	if (err != noErr) {		printf("OTUseSyncIdleEvents failed\n");		exit(1);	}	err = OTBind(endpoint, nil, nil);	if (err != noErr) {		printf("OTBind failed\n");		exit(1);	}	OTInetStringToHost("133.133.133.133", &host);	OTInitInetAddress(&peer, 1337, host);	OTMemzero(&sndCall, sizeof(TCall));	sndCall.addr.buf = (UInt8 *) &peer;	sndCall.addr.len = sizeof(InetAddress);	err = OTConnect(endpoint, &sndCall, nil);	if (err != noErr) {		printf("OTConnect failed\n");		exit(1);	}	send_str("Hello World!\n");		char c;		while (c = menu()) {		switch (c) {		case 'c':		case 'C':			AddNote();			continue;		case 'r':		case 'R':			PrintNote();			continue;		case 'u':		case 'U':			EditNote();			continue;		case 'd':		case 'D':			DeleteNote();			continue;		case 'E':		case 'e':			err = OTUnbind(endpoint);			assertp("OTUnbind failed", err == noErr);			err = OTCloseProvider(endpoint);			assertp("OTCloseProvider failed", err == noErr);		 	CloseOpenTransport();			exit(0);		}	}}