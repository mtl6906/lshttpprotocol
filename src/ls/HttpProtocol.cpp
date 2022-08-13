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

	string getHeaderEndMark(int &ec, io::InputStream &in)
	{
		LOGGER(ls::INFO) << "begin: " << in.getBuffer() -> begin() << ls::endl;
		auto text = in.split(ec, "\r\n\r\n", true);
		if(ec < 0)
			ec = Exception::LS_ENOCOMPLETE;
		return text;
	}

	http::Request* parseRequest(int &ec, const string &text)
	{
		auto request = new http::Request();
		ec = request -> parseFrom(text);
		if(ec < 0)
		{
			LOGGER(ls::INFO) << "parse failed..." << ls::endl;
			delete request;
			return nullptr;
		}
		return request;
	}

	int getBody(io::InputStream &in, http::Request *request)
	{
		int ec = Exception::LS_OK;
		auto contentLength = request -> getAttribute(ec, "Content-Length");
		if(ec < 0)
		{
			LOGGER(ls::ERROR) << "error code: " << ec << ls::endl;
			errorRequest(request, "411");
			return ec;
		}
		int len = stoi(contentLength);
		auto text = in.split(ec, contentLength);
		if(ec < 0)
		{
			ec = Exception::LS_ENOCOMPLETE;
			return ec;
		}
		request -> setBody(new http::StringBody(text, ""));
		return ec;
	}

	int HttpProtocol::readContext(rpc::Connection *connection)
	{
		int ec = Exception::LS_OK;
		auto in = connection -> getInputStream();
		ec = in.tryRead();
		if(ec < 0 && ec != Exception::LS_EWOULDBLOCK)
			return ec;
		if(connection -> request == nullptr)
		{
			string headerText = getHeaderEndMark(ec, in);
			if(ec < 0)
				return ec;
			connection -> request = parseRequest(ec, headerText);
			if(ec < 0)
				return ec;
		}
		auto request = (http::Request *)connection -> request;
		auto keepalive = request -> getAttribute(ec, "Connection");
		if(ec < 0 || keepalive != "keep-alive")
			connection -> keepalive = false;
		else
			connection -> keepalive = true;
		if(request -> getMethod() == "GET")
			return Exception::LS_OK;
		LOGGER(ls::INFO) << "request with body..." << ls::endl;
		ec = getBody(in, request);
		return ec;
	}

	int HttpProtocol::exec(rpc::Connection *connection)
	{
		int ec;
		LOGGER(ls::INFO) << "map method" << ls::endl;
		auto request = (http::Request *)connection -> request;
		auto URL = http::Url(request -> getURL());
		auto &uri = URL.uri;
		auto &dirKey = URL.part[0];
		auto &methods = methodMapper[request -> getMethod()];
		auto method = methods.find(dirKey);
	//	dynamic response
		if(method != methods.end())
		{
			LOGGER(ls::INFO) << "dynamic" << ls::endl;
			connection -> response = method -> second(request);
			putString(connection);
			return Exception::LS_OK;
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
			return Exception::LS_OK;
		}
	//	not found error response
		LOGGER(ls::INFO) << "error" << ls::endl;
		errorRequest(request, "404");
		connection -> response = methodMapper["GET"]["error"](request);
		putString(connection);
		return Exception::LS_OK;
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
		{
			delete (http::Request *)connection -> request;
			connection -> request = nullptr;
		}
		if(connection -> response != nullptr)
		{
			delete (http::Response *)connection -> response;
			connection -> response = nullptr;
		}
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
