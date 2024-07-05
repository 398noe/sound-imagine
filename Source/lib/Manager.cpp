#include "Manager.h"

Manager::Manager()
{
}

Manager::~Manager()
{
}

bool Manager::isAudioBufferReady()
{
    return processor[0].getBufferIdx() == FFT_MASK && processor[1].getBufferIdx() == FFT_SIZE;
}