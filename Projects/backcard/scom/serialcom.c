#include "stdafx.h"
#include <stdio.h>
#include "serialcom.h"

int readcom(com_t com);
int waitonreads(com_t com[], int comnum, unsigned int waitimeout_ms);

com_t com_new()
{
	com_t rt = NULL;

	while ((rt = (com_t)calloc(1, sizeof(com_st))) == NULL) Sleep(1);

	return rt;
}

int com_sess_init(comsess_t com)
{
	com->state = eState_init;

	return 1;
}

int com_open(com_t com, int comno, int baud, int ifparity, int paritype, int databits, int stopbits, onreadcb_t rcb)
{
	DCB dcb;
	wchar_t name[64] = { 0 };

	if (comno < 1 || baud <= 0) return 0;

	sprintf_s(name, sizeof(name) / sizeof(name[0]) - 1, "\\\\.\\com%d", comno);

	HANDLE hcom;
	hcom = CreateFile(name,
		GENERIC_READ | GENERIC_WRITE,
		0,
		0,
		OPEN_EXISTING,
		FILE_FLAG_OVERLAPPED,
		0);

	if (hcom == INVALID_HANDLE_VALUE) {
		DWORD dwCode = GetLastError();
		printf("open com%d falied. code is %d.\n", comno, dwCode);
		return 0;
	}

	com->hcom = hcom;
	com->comno = comno;
	com->baudrate = baud;
	com->ifparity = ifparity;
	com->paritype = paritype;
	com->databits = databits;
	com->stopbits = stopbits;

	memset(&dcb, 0, sizeof(dcb));

	if (!GetCommState(com->hcom, &dcb)){ CloseHandle(hcom); return 0; }

	dcb.BaudRate = baud;
	dcb.ByteSize = databits;
	dcb.StopBits = stopbits;
	dcb.fParity = ifparity;
	dcb.Parity = paritype;

	if (!SetCommState(com->hcom, &dcb)) { CloseHandle(hcom); return 0; }

	COMMTIMEOUTS timeouts;
	memset(&timeouts, 0, sizeof(timeouts));
	timeouts.ReadIntervalTimeout = 20;
	timeouts.ReadTotalTimeoutMultiplier = 10;
	timeouts.ReadTotalTimeoutConstant = 100;
	timeouts.WriteTotalTimeoutMultiplier = 10;
	timeouts.WriteTotalTimeoutConstant = 100;

	if (!SetCommTimeouts(com->hcom, &timeouts)) { CloseHandle(hcom); return 0; }

	com->sess.osreader.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	com->sess.oswrite.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	com->_cb = rcb;

	com->sess.state = eState_open;

	return 1;
}

void  com_close(com_t com)
{
	if (com == NULL) return;

	if (com->hcom) { CloseHandle(com->hcom); com->hcom = NULL; }
	
	if (com->sess.osreader.hEvent) { CloseHandle(com->sess.osreader.hEvent); com->sess.osreader.hEvent = NULL; }
	
	if (com->sess.oswrite.hEvent) { CloseHandle(com->sess.oswrite.hEvent); com->sess.oswrite.hEvent = NULL; }

	com->sess.state = eState_new;
}

void  com_free(com_t com)
{
	if (com == NULL) return;

	free(com);
}

void  com_app(com_t com, onreadcb_t rcb, onwritecb_t wcb)
{
	if (com == NULL) return;
	com->_cb = rcb;
}

int writecom(com_t com, char wbuffer[], int wlen)
{
	int res = FALSE;
	DWORD written = 0;

	if (com == NULL || com->sess.oswrite.hEvent == NULL || wbuffer == NULL || wlen < 1) return 0;

	// Issue write.
	if (!WriteFile(com->hcom, wbuffer, wlen, &written, &com->sess.oswrite)) {
		if (GetLastError() != ERROR_IO_PENDING) {
			// WriteFile failed, but it isn't delayed. Report error and abort.
			res = FALSE;
		}else {
			// Write is pending.
			if (!GetOverlappedResult(com->hcom, &com->sess.oswrite, &written, TRUE))
				res = FALSE;
			else{
				// Write operation completed successfully.
				res = TRUE;
				com->sess.writelen = written;
			}
		}
	}else
		// WriteFile completed immediately.
		res = TRUE;

	return res;

}

int com_run(com_t com[], int comnum, unsigned int waitimeout_ms)
{
	com_t  comarray[64] = {0};
	HANDLE events[64]   = {0};
	int    num = 0;

	if (comnum > 63) comnum = 63;

	for (int i = 0; i < comnum; i++)
	{
		if ((com[i] != NULL) && (!com[i]->sess.ifreadwating) && (com[i]->hcom != NULL))
		{
			readcom(com[i]);
		}
	}

	return waitonreads(com, comnum, waitimeout_ms);
}


int readcom(com_t com)
{
	int rt = 1;

	comsess_t sess = &com->sess;
	
	if (sess == 0 || com->hcom == NULL || sess->ifreadwating) return 0;

	if (sess->osreader.hEvent == NULL) return 0;
	// Error creating overlapped event; abort.

	// Issue read operation.
	if (!ReadFile(com->hcom, sess->rbuffer, sizeof(sess->rbuffer), &sess->readlen, &sess->osreader))
	{
		if (GetLastError() != ERROR_IO_PENDING)     // read not delayed?
		{
			sess->readlen = 0;
			if (com->_cb) com->_cb(sess->rbuffer, sess->readlen, eCode_err, com);//read bytes is 0.
			// Error in communications; report it.
			rt = 0;
		}else{
			sess->ifreadwating = TRUE;
		}
	}else {
		// read complete immediately
		if (com->_cb) com->_cb(sess->rbuffer, sess->readlen, 0, com);
	}

	return rt;
}

int waitonreads(com_t com[], int comnum, unsigned int waitimeout_ms)
{
	int res = 0;

	com_t comarray[64] = {0};
	HANDLE events [64] = {0};
	int   num = 0;

	if (comnum > 63) comnum = 63;

	for (int i = 0; i < comnum; i++)
	{
		if (com[i] && com[i]->hcom != NULL && com[i]->sess.ifreadwating)
		{
			comarray[num] = com[i];
			events[num++] = com[i]->sess.osreader.hEvent;
		}
	}
	if (num == 0) return res;
	
	DWORD rslt = WaitForMultipleObjects(num, events, FALSE, waitimeout_ms);
	switch (rslt)
	{
		case WAIT_TIMEOUT:  break;
		case WAIT_FAILED :  break;
		case WAIT_ABANDONED:break;
		default:
		{
			int ndx = rslt - WAIT_OBJECT_0;
			if (ndx >= 0 && ndx < num)
			{
				com_t evtcom = comarray[ndx];

				BOOL rslt = GetOverlappedResult(evtcom->hcom, &evtcom->sess.osreader, &evtcom->sess.readlen, FALSE);
				if (rslt)
				{
					if (evtcom->_cb) evtcom->_cb(evtcom->sess.rbuffer, evtcom->sess.readlen, 0, evtcom);
					res = 1;
				}
				
				evtcom->sess.ifreadwating = FALSE;
			}
			break;
		}
	}

	return res;	
}