#include "ls/HttpProtocol.h"
#include "ls/http/Url.h"
#include "ls/file/API.h"
#include "ls/http/Request.h"
#include "ls/http/Response.h"

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

	int getBody(io::InputStream &in, http::Request *request)
	{
		int ec = Exception::LS_OK;
		auto contentLength = request -> getAttribute("Content-Length");
		if(contentLength == "")
		{
			LOGGER(ls::ERROR) << "error code: " << ec << ls::endl;
			errorRequest(request, "411");
			return Exception::LS_ENOCONTENT;
		}
		int len = stoi(contentLength);
		auto text = in.split(ec, contentLength);
		if(ec < 0)
		{
			ec = Exception::LS_ENOCOMPLETE;
			return ec;
		}
		request -> getBody() = std::move(text);
		return ec;
	}

	int HttpProtocol::readContext(rpc::Connection *connection)
	{
		int ec = Exception::LS_OK;
		auto in = connection -> getInputStream();
		ec = in.tryRead();
		if(ec < 0 && ec != Exception::LS_EWOULDBLOCK)
			return ec;
		if(request.getMethod() == "")
		{
			string headerText = getHeaderEndMark(ec, in);
			if(ec < 0)
				return ec;
			ec = request.parseFrom(headerText);
			if(ec < 0)
				return ec;
		}
		auto keepalive = request.getConnection();
		if(keepalive != "keep-alive")
			connection -> keepalive = false;
		else
			connection -> keepalive = true;
		LOGGER(ls::INFO) << connection -> keepalive << endl;
		if(request.getMethod() == "GET")
			return Exception::LS_OK;
		LOGGER(ls::INFO) << "request with body..." << ls::endl;
		ec = getBody(in, &request);
		return ec;
	}

	int HttpProtocol::exec(rpc::Connection *connection)
	{
		int ec;
		LOGGER(ls::INFO) << "map method" << ls::endl;
		auto URL = http::Url(request.getURL());
		auto &uri = URL.uri;
		auto &dirKey = URL.part[0];
	//	fast file
		auto fastfileIt = fastfileMapper.find(dirKey);
		if(fastfileIt != fastfileMapper.end())
		{
			connection -> sendBuffer.push(fastfileIt -> second);
			connection -> file.reset(nullptr);
			LOGGER(ls::INFO) << fastfileIt -> second << ls::endl;
			return Exception::LS_OK;	
		}
		auto &methods = methodMapper[request.getMethod()];
		auto method = methods.find(dirKey);
	//	dynamic response
		response.reset(&connection -> sendBuffer);
		if(method != methods.end())
		{
			LOGGER(ls::INFO) << "dynamic" << ls::endl;
			method -> second(request, response);
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
			response.setResponseLine("200", request.getVersion());
			response.setHeaderByRequest(request);
			response.setFileBody(uri);
			putFile(connection);
			return Exception::LS_OK;
		}
	//	not found error response
		LOGGER(ls::INFO) << "error" << ls::endl;
		errorRequest(&request, "404");
		methodMapper["GET"]["error"](request, response);
		putString(connection);
		return Exception::LS_OK;
	}

	void HttpProtocol::putFile(rpc::Connection *connection)
	{
		connection -> file.reset(response.getFile());
	}

	void HttpProtocol::putString(rpc::Connection *connection)
	{
//		connection -> staticSendBuffer -> push("Content-Type: text/plain\r\n");
//		connection -> staticSendBuffer -> push("Content-Length: 10\r\n\r\nhelloworld");
		connection -> file.reset(nullptr);
	}

	void HttpProtocol::add(const string &method, const string &key, int(*func)(http::Request &request, http::Response &response))
	{
		methodMapper[method][key] = func;
	}

	void HttpProtocol::add(const string &key, const string &dir)
	{
		auto value = dir.substr(1);
		dirMapper[key] = value;
		LOGGER(ls::INFO) << "add directory [" << key << "," << value << "]" << ls::endl;
	}	

	void HttpProtocol::addfile(const string &key, const string &path)
	{
		if(file::api.exist(path) == true)
		{
			auto file = file::api.get(path);
			Buffer buffer(8096);
			io::InputStream in(file -> getReader(), &buffer);
			in.read();	
			auto data = in.split();
			http::Response _response;
			_response.reset(&buffer);
			_response.setResponseLine("200", "HTTP/1.1");
			_response.setAttribute("Connection", "keep-alive");
			_response.setStringBody(data, "text/plain");
			fastfileMapper[key] = in.split();
		}
	}

	void HttpProtocol::release(rpc::Connection *connection)
	{
		LOGGER(ls::INFO) << "release request and response" << ls::endl;
		connection -> file.reset(nullptr);
		request.clear();
		response.clear();	
	}

	file::File *HttpProtocol::getFile(rpc::Connection *connection)
	{
		return connection -> file.get();
	}
}
