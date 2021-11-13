#include "ls/HttpProtocol.h"
#include "ls/http/Url.h"
#include "ls/http/StringBody.h"
#include "ls/http/FileBody.h"
#include "ls/file/API.h"


using namespace std;

namespace ls
{
	HttpProtocol::HttpProtocol(const string &tag, int port) : Protocol(tag, port)
	{

	}

	HttpProtocol::~HttpProtocol()
	{
		
	}

	void errorRequest(http::Request *request, const string &code)
	{
		request -> setDefaultHeader();
		request -> getMethod() = "GET";
		request -> getURL() = string("/error?code=") + code;
	}

	string getHeaderEndMark(io::InputStream &in)
	{
		try
		{
			LOGGER(ls::INFO) << "begin: " << in.getBuffer() -> begin() << ls::endl;
			return in.split("\r\n\r\n", true);
		}
		catch(Exception &e)
		{
			throw Exception(Exception::LS_ENOCOMPLETE);
		}
	}

	http::Request* parseRequest(const string &text)
	{
		auto request = new http::Request();
		try
		{
			request -> parseFrom(text);
		}
		catch(Exception &e)
		{
			LOGGER(ls::INFO) << "parse failed..." << ls::endl;
			delete request;
			throw e;
		}
		return request;
	}

	void getBody(io::InputStream &in, http::Request *request)
	{
		int contentLength = 0;
		try
		{
			stoi(request -> getAttribute("Content-Length"));
		}
		catch(Exception &e)
		{
			errorRequest(request, "411");
			return ;
		}
		try
		{
			auto text = in.split(contentLength);
			request -> setBody(new http::StringBody(text, ""));
		}
		catch(Exception &e)
		{
			throw Exception(Exception::LS_ENOCOMPLETE);
		}
	}

	void HttpProtocol::readContext(rpc::Connection *connection)
	{
		connection -> isRelease = true;
		auto in = connection -> getInputStream();
		try
		{
			in.tryRead();
		}
		catch(Exception &e)
		{
			if(e.getCode() != Exception::LS_EWOULDBLOCK)
				throw e;
		}
		if(connection -> request == nullptr)
		{
			string headerText = getHeaderEndMark(in);
			connection -> request = parseRequest(headerText);
		}
		auto request = (http::Request *)connection -> request;
		if(request -> getMethod() == "GET")
			return;
		LOGGER(ls::INFO) << "request with body..." << ls::endl;
		getBody(in, request);
	}

	void HttpProtocol::exec(rpc::Connection *connection)
	{
		LOGGER(ls::INFO) << "map method" << ls::endl;
		auto request = (http::Request *)connection -> request;
		auto URL = http::Url(request -> getURL());
		auto &uri = URL.uri;
		auto &dirKey = URL.part[0];
		auto &methods = methodMapper[request -> getMethod()];
		auto method = methods.find(uri);
	//	dynamic response
		if(method != methods.end())
		{
			LOGGER(ls::INFO) << "dynamic" << ls::endl;
			connection -> response = method -> second(request);
			putString(connection);
			return;
		}
	//	static response
		string dir;
		auto dirIt = dirMapper.end();
		if(URL.part.size() > 0)
			dirIt = dirMapper.find(URL.part[0]);
		if(dirIt != dirMapper.end())
		{
			dir = dirIt -> second;
 			URL.part[0] = dir;
			URL.reset(URL.part, URL.queryText);
			LOGGER(ls::INFO) << "URL.url: " << URL.url << ls::endl;
		}
		LOGGER(ls::INFO) << dir << " " << file::api.exist(URL.uri) << ls::endl;
		if(dir != "" && file::api.exist(URL.uri))
		{
			LOGGER(ls::INFO) << "static" << ls::endl;
			auto response = new http::Response();
			response -> setDefaultHeader(*request);
			response -> setCode("200");
			response -> setBody(new http::FileBody(uri));
			connection -> response = response;
			putFile(connection);
			return;
		}
	//	not found error response
		LOGGER(ls::INFO) << "error" << ls::endl;
		errorRequest(request, "404");
		connection -> response = methodMapper["GET"]["/error"](request);
		putString(connection);
	}

	void HttpProtocol::putFile(rpc::Connection *connection)
	{
		auto response = (http::Response *)connection -> response;
		auto header = response -> toString();
		connection -> staticSendBuffer -> push(header);
		connection -> responseType = "static";
	}

	void HttpProtocol::putString(rpc::Connection *connection)
	{
		auto response = (http::Response *)connection -> response;
		auto header = response -> toString();
		connection -> staticSendBuffer -> push(header);
		auto *body = response -> getBody();
		string text;
		body -> getData(&text);
		connection -> dynamicSendBuffer = new Buffer(text);
		connection -> responseType = "dynamic";
	}

	void HttpProtocol::add(const string &method, const string &key, http::Response *(*func)(http::Request *request))
	{
		methodMapper[method][key] = func;
	}

	void HttpProtocol::add(const string &key, const string &dir)
	{
		auto value = dir.substr(1);
		dirMapper[key] = value;
		LOGGER(ls::INFO) << "add directory [" << key << "," << value << "]" << ls::endl;
	}	

	void HttpProtocol::release(rpc::Connection *connection)
	{
		LOGGER(ls::INFO) << "release request and response" << ls::endl;
		if(connection -> request != nullptr)
			delete (http::Request *)connection -> request;
		if(connection -> response != nullptr)
			delete (http::Response *)connection -> response;
	}

	file::File *HttpProtocol::getFile(rpc::Connection *connection)
	{
		auto response = (http::Response *)connection -> response;
		auto *body = response -> getBody();
		file::File *file;
		body -> getData(&file);
		return file;
	}
}
