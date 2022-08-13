#include "ls/rpc/Tool.h"
#include "ls/HttpProtocol.h"
#include "ls/http/Response.h"
#include "ls/http/QueryString.h"
#include "ls/http/Url.h"
#include "ls/http/StringBody.h"
#include "ls/file/API.h"
#include "CASQueueFactory.h"

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

http::Response *getTable(http::Request *request)
{
	http::Response *response = new http::Response();
	response -> setDefaultHeader(*request);
	response -> setCode("200");

	unique_ptr<file::File> file(file::api.get("paste_data.txt"));
	Buffer readBuffer(file -> size());
	io::InputStream in(file -> getReader(), &readBuffer);
	in.read();
	string data = in.split();
	response -> setBody(new http::StringBody(data, "text/html"));
	return response;
}

http::Response *hello2(http::Request *request)
{
	static auto response = new http::Response();
	if(response)
		return response;
	response -> setDefaultHeader(*request);
	response -> setCode("200");
	response -> setBody(new http::StringBody("hello", "text/plain"));
	return response;
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
http::Response *error(http::Request *request)
{
	int ec;
	http::QueryString qs;
	auto queryText = http::Url(request -> getURL()).queryText;
	string code;
	while(queryText != "")
	{
		ec = qs.parseFrom(queryText);
		if(ec < 0)
		{
			code = "400";
			break;
		}
		code = qs.getParameter(ec, "code");
		if(ec < 0)
		{
			code = "400";
			break;
		}
	}
	auto response = new http::Response();
	response -> setDefaultHeader(*request);
	response -> setCode(code);
	response -> setBody(new http::StringBody(code, "text/plain"));
	return response;
}

int main(int argc, char **argv)
{
	cout << 1 << endl;
	int port = stoi(argv[1]);
	CASQueueFactory casqf;
	rpc::Tool tool(casqf);
	for(int i=0;i<1;++i)
	{
	auto hp = new HttpProtocol("http" + to_string(i), port);
	hp -> add("GET", "hello", hello);
	hp -> add("GET", "error", error);
	hp -> add("GET", "table", getTable);
//	hp -> add("POST", "pasteboard", pasteboard);
	hp -> add("hello1", "/home/mtl/old/mtl/code/lib/HttpProtocol/build");
	hp -> add("GET", "hello2", hello2);
//	hp -> add("POST", "signUp", signUp);
//	hp -> add("POST", "sendCode", sendCode);
//	hp -> add("POST", "signIn", signIn);
	tool.push(hp);
	}
	tool.run();
	return 0;
}
