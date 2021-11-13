#ifndef LS_HTTPPROTOCOL_H
#define LS_HTTPPROTOCOL_H

#include "ls/rpc/Protocol.h"
#include "ls/http/Request.h"
#include "ls/http/Response.h"
#include "map"
#include "string"

namespace ls
{
	class HttpProtocol : public rpc::Protocol
	{
		public:
			HttpProtocol(const std::string &tag, int port);
			~HttpProtocol() override;
			void readContext(rpc::Connection *connection) override;
			void exec(rpc::Connection *connection) override;
			void release(rpc::Connection *connection) override;
			void putString(rpc::Connection *connection) override;
			void putFile(rpc::Connection *connection) override;
			file::File *getFile(rpc::Connection *connection) override;
			void add(const std::string &method, const std::string &key, http::Response*(*func)(http::Request *request));
			void add(const std::string &key, const std::string &dir);
		protected:
			std::map<std::string, std::map<std::string, http::Response*(*)(http::Request *)>> methodMapper;
			std::map<std::string, std::string> dirMapper;
			Pool<Buffer> bufferPool;
	};
}

#endif
