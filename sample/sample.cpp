#include "ls/rpc/Tool.h"
#include "ls/HttpProtocol.h"
#include "ls/http/Response.h"
#include "ls/http/QueryString.h"
#include "ls/http/Url.h"
#include "ls/http/StringBody.h"

using namespace ls;
using namespace std; 

http::Response* hello(http::Request *request)
{
	static int count = 0;
	string text = string("hello, ") + to_string(++count);
	auto response = new http::Response();
	response -> setDefaultHeader(*request);
	response -> setCode("200");
	response -> setBody(new http::StringBody(text, "text/plain"));
	return response;
}

http::Response *error(http::Request *request)
{
	http::QueryString qs;
	auto queryText = http::Url(request -> getURL()).queryText;
	string code;
	try
	{
		if(queryText != "")
		{
			qs.parseFrom(queryText);
			code = qs.getParameter("code");
		}
		else
			code = "400";
	}
	catch(Exception &e)
	{
		code = "400";
	}
	auto response = new http::Response();
	response -> setDefaultHeader(*request);
	response -> setCode(code);
	response -> setBody(new http::StringBody(code, "text/plain"));
	return response;
}

int main(int argc, char **argv)
{
	int port = stoi(argv[1]);
	auto hp = new HttpProtocol("http", port);
	hp -> add("GET", "/hello", hello);
	hp -> add("GET", "/error", error);
	hp -> add("hello", "/home/mtl/old/mtl/code/lib/HttpProtocol/build");
	rpc::Tool tool;
	tool.push(hp);
	tool.run();
	return 0;
}
