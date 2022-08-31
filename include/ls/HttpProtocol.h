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
			int readContext(rpc::Connection *connection) override;
			int exec(rpc::Connection *connection) override;
			void release(rpc::Connection *connection) override;
			void putString(rpc::Connection *connection) override;
			void putFile(rpc::Connection *connection) override;
			file::File *getFile(rpc::Connection *connection) override;
			void add(const std::string &method, const std::string &key, 
					int(*func)(
						http::Request &request,
						http::Response &response
					)
			);
			void add(const std::string &key, const std::string &dir);
			void addfile(const std::string &key, const std::string &data);
		protected:
			std::map<std::string, std::map<std::string, int(*)(http::Request &, http::Response &)>> methodMapper;
			std::map<std::string, std::string> dirMapper;
			std::map<std::string, std::string> fastfileMapper;
			http::Response response;
			http::Request request;
	};

	void errorRequest(http::Request *request, const std::string &code);
}

#endif
