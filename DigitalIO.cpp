#include "DigitalIO.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <sys/time.h>
#include <iostream>
#include <string>
#include <unistd.h>

void DigitalIO::Init() {

	// Declare gpio pins
    this->systemReadyOut	= gpio36; // SystemReady
    this->goodOut 			= gpio37; // Good
    this->badOut 			= gpio38; // Bad

    // Make the outputs available in user space
    gpioExport(this->systemReadyOut);
    gpioExport(this->goodOut);
    gpioExport(this->badOut);

	// Set direction as outputs
    gpioSetDirection(this->systemReadyOut,outputPin);
    gpioSetDirection(this->goodOut,outputPin);
    gpioSetDirection(this->badOut,outputPin);	
}

void DigitalIO::Shutdown() {

    gpioUnexport(this->systemReadyOut);
    gpioUnexport(this->goodOut);
    gpioUnexport(this->badOut);
}

void DigitalIO::SetSystemReady()
{
	gpioSetValue(this->systemReadyOut, on);
}

void DigitalIO::ClearSystemReady()
{
	gpioSetValue(this->systemReadyOut, off);
}

void DigitalIO::SetGood()
{
	gpioSetValue(this->goodOut, on);
}

void DigitalIO::ClearGood()
{
	gpioSetValue(this->goodOut, off);
}

void DigitalIO::SetBad()
{
	gpioSetValue(this->badOut, on);
}

void DigitalIO::ClearBad()
{
	gpioSetValue(this->badOut, off);
}


