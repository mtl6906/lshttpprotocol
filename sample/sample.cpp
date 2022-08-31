#include "ls/rpc/Tool.h"
#include "ls/HttpProtocol.h"
#include "ls/http/Response.h"
#include "ls/http/QueryString.h"
#include "ls/http/Url.h"
#include "ls/file/API.h"
#include "CASQueueFactory.h"

using namespace ls;
using namespace std; 

int hello(http::Request &request, http::Response &response)
{
	response.setResponseLine("200", request.getVersion());
	response.setHeaderByRequest(request);
	response.setStringBody("helloworld", "text/plain");
		
	return Exception::LS_OK;
}

int getTable(http::Request &request, http::Response &response)
{
	unique_ptr<file::File> file(file::api.get("paste_data.txt"));
	Buffer readBuffer(file -> size());
	io::InputStream in(file -> getReader(), &readBuffer);
	in.read();
	string data = in.split();
	response.setResponseLine("200", request.getVersion());
	response.setHeaderByRequest(request);
	response.setStringBody(data, "text/html");
	return Exception::LS_OK;
}

string resp(Exception &e)
{
	json::Object resp;
	json::api.push(resp, "code", e.getCode());
	json::api.push(resp, "message", string(e.what()));
	return json::api.encode(resp);
}
/*
http::Response *pasteboard(http::Request *request)
{
	string body;
	request -> getBody() -> getData(&body);
	http::Response *response = new http::Response();
	response -> setDefaultHeader(*request);
	response -> setCode("200");
	string data;
	try
	{
		json::Object root = json::api.decode(body);
		json::api.get(root, "data", data);
	}
	catch(Exception &e)
	{
		response -> setBody(new http::StringBody(resp(e), "text/html"));
		return response;
	}

	string fileData;
	try
	{
		unique_ptr<file::File> file(file::api.get("paste_data.txt"));
		Buffer readBuffer(file->size());
		io::InputStream in(file -> getReader(), &readBuffer);
		in.read();
		fileData = in.split();
	}
	catch(ls::Exception &e)
	{
		if(e.getCode() != Exception::LS_EEOF)
		{
			response -> setBody(new http::StringBody(resp(e), "text/html"));
			return response;
		}		
	}
	LOGGER(ls::INFO) << fileData << ls::endl;
	json::Object root = json::api.decode(fileData);
	json::Array pasteboard;
	json::api.get(root, "pasteboard", pasteboard);
	LOGGER(ls::INFO) << "get ok" << ls::endl;
	json::api.push(pasteboard, data);
	LOGGER(ls::INFO) << "push ok" << ls::endl;
	json::api.replace(root, "pasteboard", pasteboard);
	LOGGER(ls::INFO) << "replace ok" << ls::endl;
	string result = json::api.encode(root);
	unique_ptr<file::File> file(file::api.get("paste_data.txt"));
	Buffer writeBuffer(result);
	io::OutputStream out(file -> getWriter(), &writeBuffer);
	out.write();
	response -> setBody(new http::StringBody("{\"code\": 0}", "text/html"));
	return response;
}
*/
int error(http::Request &request, http::Response &response)
{
	int ec;
	http::QueryString qs;
	auto queryText = http::Url(request.getURL()).queryText;
	string code;
	ec = qs.parseFrom(queryText);
	code = qs.getParameter("code");
	response.setResponseLine(code, request.getVersion());
	response.setHeaderByRequest(request);
	response.setStringBody(code, "text/plain");
	return Exception::LS_OK;
}

int main(int argc, char **argv)
{
	cout << 1 << endl;
	int port = stoi(argv[1]);
	CASQueueFactory casqf;
	rpc::Tool tool(casqf);
	for(int i=0;i<20;++i)
	{
	auto hp = new HttpProtocol("http" + to_string(i), port);
	hp -> add("GET", "hello", hello);
	hp -> add("GET", "error", error);
	hp -> add("GET", "table", getTable);
//	hp -> add("POST", "pasteboard", pasteboard);
	hp -> add("hello1", "/home/mtl/old/mtl/code/lib/HttpProtocol/build");
	hp -> addfile("hello2", "config.json");
//	hp -> add("POST", "signUp", signUp);
//	hp -> add("POST", "sendCode", sendCode);
//	hp -> add("POST", "signIn", signIn);
	tool.push(hp);
	}
	tool.run();
	return 0;
}
