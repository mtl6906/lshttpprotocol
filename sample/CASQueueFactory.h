#ifndef CASQUEUEFACTORY_H
#define CASQUEUEFACTORY_H

#include "ls/rpc/QueueFactory.h"


class CASQueueFactory : public ls::rpc::QueueFactory
{
	public:
		~CASQueueFactory() override {}
		ls::Queue<ls::rpc::Connection *> *makeQueue(int size) override;
};

#endif
