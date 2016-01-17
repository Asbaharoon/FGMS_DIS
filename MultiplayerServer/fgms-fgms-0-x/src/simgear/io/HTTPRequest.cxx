#include "HTTPRequest.hxx"

#include <simgear/compiler.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/structure/exception.hxx>

namespace simgear
{
namespace HTTP
{

extern const int DEFAULT_HTTP_PORT;

//------------------------------------------------------------------------------
Request::Request(const std::string& url, const std::string method):
  _method(method),
  _url(url),
  _responseVersion(HTTP_VERSION_UNKNOWN),
  _responseStatus(0),
  _responseLength(0),
  _receivedBodyBytes(0),
  _ready_state(UNSENT),
  _willClose(false)
{

}

//------------------------------------------------------------------------------
Request::~Request()
{

}

//------------------------------------------------------------------------------
Request* Request::done(const Callback& cb)
{
  if( _ready_state == DONE )
    cb(this);
  else
    _cb_done = cb;

  return this;
}

//------------------------------------------------------------------------------
Request* Request::fail(const Callback& cb)
{
  if( _ready_state == FAILED )
    cb(this);
  else
    _cb_fail = cb;

  return this;
}

//------------------------------------------------------------------------------
Request* Request::always(const Callback& cb)
{
  if( isComplete() )
    cb(this);
  else
    _cb_always = cb;

  return this;
}

//------------------------------------------------------------------------------
void Request::setBodyData( const std::string& data,
                           const std::string& type )
{
  _request_data = data;
  _request_media_type = type;

  if( !data.empty() && _method == "GET" )
    _method = "POST";
}

//----------------------------------------------------------------------------
void Request::setBodyData(const SGPropertyNode* data)
{
  if( !data )
    setBodyData("");

  std::stringstream buf;
  writeProperties(buf, data, true);

  setBodyData(buf.str(), "application/xml");
}

//------------------------------------------------------------------------------
void Request::setUrl(const std::string& url)
{
  _url = url;
}

//------------------------------------------------------------------------------
void Request::requestStart()
{
  setReadyState(OPENED);
}

//------------------------------------------------------------------------------
Request::HTTPVersion decodeHTTPVersion(const std::string& v)
{
  if( v == "HTTP/1.1" ) return Request::HTTP_1_1;
  if( v == "HTTP/1.0" ) return Request::HTTP_1_0;
  if( strutils::starts_with(v, "HTTP/0.") ) return Request::HTTP_0_x;
  return Request::HTTP_VERSION_UNKNOWN;
}

//------------------------------------------------------------------------------
void Request::responseStart(const std::string& r)
{
    const int maxSplit = 2; // HTTP/1.1 nnn reason-string
    string_list parts = strutils::split(r, NULL, maxSplit);
    if (parts.size() != 3) {
        throw sg_io_exception("bad HTTP response");
    }
    
    _responseVersion = decodeHTTPVersion(parts[0]);
    _responseStatus = strutils::to_int(parts[1]);
    _responseReason = parts[2];
}

//------------------------------------------------------------------------------
void Request::responseHeader(const std::string& key, const std::string& value)
{
  if( key == "connection" )
    _willClose = (value.find("close") != std::string::npos);

  _responseHeaders[key] = value;
}

//------------------------------------------------------------------------------
void Request::responseHeadersComplete()
{
  setReadyState(HEADERS_RECEIVED);
}

//------------------------------------------------------------------------------
void Request::responseComplete()
{
  if( !isComplete() )
    setReadyState(DONE);
}

//------------------------------------------------------------------------------
void Request::gotBodyData(const char* s, int n)
{
  setReadyState(LOADING);
}

//------------------------------------------------------------------------------
void Request::onDone()
{

}

//------------------------------------------------------------------------------
void Request::onFail()
{
  SG_LOG
  (
    SG_IO,
    SG_INFO,
    "request failed:" << url() << " : "
                      << responseCode() << "/" << responseReason()
  );
}

//------------------------------------------------------------------------------
void Request::onAlways()
{

}

//------------------------------------------------------------------------------
void Request::processBodyBytes(const char* s, int n)
{
  _receivedBodyBytes += n;
  gotBodyData(s, n);
}

//------------------------------------------------------------------------------
std::string Request::scheme() const
{
    int firstColon = url().find(":");
    if (firstColon > 0) {
        return url().substr(0, firstColon);
    }
    
    return ""; // couldn't parse scheme
}

//------------------------------------------------------------------------------
std::string Request::path() const
{
    std::string u(url());
    int schemeEnd = u.find("://");
    if (schemeEnd < 0) {
        return ""; // couldn't parse scheme
    }
    
    int hostEnd = u.find('/', schemeEnd + 3);
    if (hostEnd < 0) {
// couldn't parse host, or URL looks like 'http://foo.com' (no trailing '/') 
// fixup to root resource path: '/' 
        return "/"; 
    }
    
    int query = u.find('?', hostEnd + 1);
    if (query < 0) {
        // all remainder of URL is path
        return u.substr(hostEnd);
    }
    
    return u.substr(hostEnd, query - hostEnd);
}

//------------------------------------------------------------------------------
std::string Request::query() const
{
  std::string u(url());
  int query = u.find('?');
  if (query < 0) {
    return "";  //no query string found
  }
  
  return u.substr(query);   //includes question mark
}

//------------------------------------------------------------------------------
std::string Request::host() const
{
  std::string hp(hostAndPort());
  int colonPos = hp.find(':');
  if (colonPos >= 0) {
      return hp.substr(0, colonPos); // trim off the colon and port
  } else {
      return hp; // no port specifier
  }
}

//------------------------------------------------------------------------------
unsigned short Request::port() const
{
  std::string hp(hostAndPort());
  int colonPos = hp.find(':');
  if (colonPos >= 0) {
      return (unsigned short) strutils::to_int(hp.substr(colonPos + 1));
  } else {
      return DEFAULT_HTTP_PORT;
  }
}

//------------------------------------------------------------------------------
std::string Request::hostAndPort() const
{
  std::string u(url());
  int schemeEnd = u.find("://");
  if (schemeEnd < 0) {
      return ""; // couldn't parse scheme
  }

  int hostEnd = u.find('/', schemeEnd + 3);
  if (hostEnd < 0) { // all remainder of URL is host
      return u.substr(schemeEnd + 3);
  }

  return u.substr(schemeEnd + 3, hostEnd - (schemeEnd + 3));
}

//------------------------------------------------------------------------------
void Request::setResponseLength(unsigned int l)
{
  _responseLength = l;
}

//------------------------------------------------------------------------------
unsigned int Request::responseLength() const
{
  // if the server didn't supply a content length, use the number
  // of bytes we actually received (so far)
  if( (_responseLength == 0) && (_receivedBodyBytes > 0) )
    return _receivedBodyBytes;

  return _responseLength;
}

//------------------------------------------------------------------------------
void Request::setFailure(int code, const std::string& reason)
{
  _responseStatus = code;
  _responseReason = reason;
  setReadyState(FAILED);
}

//------------------------------------------------------------------------------
void Request::setReadyState(ReadyState state)
{
  _ready_state = state;
  if( state == DONE )
  {
    // Finish C++ part of request to ensure everything is finished (for example
    // files and streams are closed) before calling any callback (possibly using
    // such files)
    onDone();
    onAlways();

    if( _cb_done )
      _cb_done(this);
  }
  else if( state == FAILED )
  {
    onFail();
    onAlways();

    if( _cb_fail )
      _cb_fail(this);
  }
  else
    return;

  if( _cb_always )
    _cb_always(this);
}

//------------------------------------------------------------------------------
void Request::abort()
{
  abort("Request aborted.");
}

//----------------------------------------------------------------------------
void Request::abort(const std::string& reason)
{
  if( isComplete() )
    return;

  setFailure(-1, reason);
  _willClose = true;
}

//------------------------------------------------------------------------------
bool Request::closeAfterComplete() const
{
  // for non HTTP/1.1 connections, assume server closes
  return _willClose || (_responseVersion != HTTP_1_1);
}

//------------------------------------------------------------------------------
bool Request::isComplete() const
{
  return _ready_state == DONE || _ready_state == FAILED;
}

//------------------------------------------------------------------------------
bool Request::hasBodyData() const
{
  return !_request_media_type.empty();
}

//------------------------------------------------------------------------------
std::string Request::bodyType() const
{
  return _request_media_type;
}

//------------------------------------------------------------------------------
size_t Request::bodyLength() const
{
  return _request_data.length();
}

//------------------------------------------------------------------------------
size_t Request::getBodyData(char* s, size_t offset, size_t max_count) const
{
  size_t bytes_available = _request_data.size() - offset;
  size_t bytes_to_read = std::min(bytes_available, max_count);

  memcpy(s, _request_data.data() + offset, bytes_to_read);

  return bytes_to_read;
}

} // of namespace HTTP
} // of namespace simgear
