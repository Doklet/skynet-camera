#ifndef __DIGITAL_IO_H__
#define __DIGITAL_IO_H__

extern "C" {
	#include "jetsonGPIO.h"
}

class DigitalIO
{
public:

	void Init();
	void Shutdown();

	void SetSystemReady();
	void ClearSystemReady();

	void SetGood();
	void ClearGood();

	void SetBad();
	void ClearBad();

private:

    jetsonTX1GPIONumber systemReadyOut;
    jetsonTX1GPIONumber goodOut;
    jetsonTX1GPIONumber badOut;
};

#endif
