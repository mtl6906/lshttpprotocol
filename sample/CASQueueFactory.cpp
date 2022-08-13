#include "CASQueueFactory.h"
#include "ls/CASQueue.h"

using namespace ls;

Queue<rpc::Connection *> *CASQueueFactory::makeQueue(int size)
{
	return new CASQueue<rpc::Connection *>(size);
}
