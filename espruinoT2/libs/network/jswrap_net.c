/*
 * This file is part of Espruino, a JavaScript interpreter for Microcontrollers
 *
 * Copyright (C) 2014 Gordon Williams <gw@pur3.co.uk>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * ----------------------------------------------------------------------------
 * This file is designed to be parsed during the build process
 *
 * Contains JavaScript Net (socket) functions
 * ----------------------------------------------------------------------------
 */
#include "jswrap_net.h"
#include "jsvariterator.h"
#include "socketserver.h"
#include "network.h"

/*JSON{
  "type" : "idle",
  "generate" : "jswrap_net_idle"
}*/
bool jswrap_net_idle() {
  JsNetwork net;
  if (!networkGetFromVar(&net)) return false;
  net.idle(&net);
  bool b = socketIdle(&net);
  networkFree(&net);
  return b;
}

/*JSON{
  "type" : "init",
  "generate" : "jswrap_net_init"
}*/
void jswrap_net_init() {
  socketInit();
}

/*JSON{
  "type" : "kill",
  "generate" : "jswrap_net_kill"
}*/
void jswrap_net_kill() {
  JsNetwork net;
  if (networkWasCreated()) {
    if (!networkGetFromVar(&net)) return;
    socketKill(&net);
    networkFree(&net);
  }
}


// ---------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------

/*JSON{
  "type" : "class",
  "class" : "url"
}
This class helps to convert URLs into Objects of information ready for http.request/get
*/


/*JSON{
  "type" : "staticmethod",
  "class" : "url",
  "name" : "parse",
  "generate" : "jswrap_url_parse",
  "params" : [
    ["urlStr","JsVar","A URL to be parsed"],
    ["parseQuery","bool","Whether to parse the query string into an object not (default = false)"]
  ],
  "return" : ["JsVar","An object containing options for ```http.request``` or ```http.get```. Contains `method`, `host`, `path`, `pathname`, `search`, `port` and `query`"]
}
A utility function to split a URL into parts

This is useful in web servers for instance when handling a request.

For instance `url.parse("/a?b=c&d=e",true)` returns `{"method":"GET","host":"","path":"/a?b=c&d=e","pathname":"/a","search":"?b=c&d=e","port":80,"query":{"b":"c","d":"e"}}`
*/
JsVar *jswrap_url_parse(JsVar *url, bool parseQuery) {
  if (!jsvIsString(url)) return 0;
  JsVar *obj = jsvNewWithFlags(JSV_OBJECT);
  if (!obj) return 0; // out of memory

  // scan string to try and pick stuff out
  JsvStringIterator it;
  jsvStringIteratorNew(&it, url, 0);
  int slashes = 0;
  int colons = 0;
  int addrStart = -1;
  int portStart = -1;
  int pathStart = -1;
  int searchStart = -1;
  int charIdx = 0;
  int portNumber = 0;
  while (jsvStringIteratorHasChar(&it)) {
    char ch = jsvStringIteratorGetChar(&it);
    if (ch == '/') {
      slashes++;
      if (pathStart<0) pathStart = charIdx;
      if (colons==1 && slashes==2 && addrStart<0) {
        addrStart = charIdx;
        pathStart = -1;
        searchStart = -1;
      }
    }
    if (ch == ':') {
      colons++;
      if (addrStart>=0 && pathStart<0)
        portStart = charIdx;
    }

    if (portStart>=0 && charIdx>portStart && pathStart<0 && ch >= '0' && ch <= '9') {
      portNumber = portNumber*10 + (ch-'0');
    }

    if (ch == '?' && pathStart>=0) {
      searchStart = charIdx;
    }

    jsvStringIteratorNext(&it);
    charIdx++;
  }
  jsvStringIteratorFree(&it);
  // try and sort stuff out
  if (pathStart<0) pathStart = charIdx;
  if (pathStart<0) pathStart = charIdx;
  int addrEnd = (portStart>=0) ? portStart : pathStart;
  // pull out details
  jsvUnLock(jsvObjectSetChild(obj, "method", jsvNewFromString("GET")));
  jsvUnLock(jsvObjectSetChild(obj, "host", jsvNewFromStringVar(url, (size_t)(addrStart+1), (size_t)(addrEnd-(addrStart+1)))));

  JsVar *v;

  v = jsvNewFromStringVar(url, (size_t)pathStart, JSVAPPENDSTRINGVAR_MAXLENGTH);
  if (jsvGetStringLength(v)==0) jsvAppendString(v, "/");
  jsvUnLock(jsvObjectSetChild(obj, "path", v));

  v = jsvNewFromStringVar(url, (size_t)pathStart, (size_t)((searchStart>=0)?(searchStart-pathStart):JSVAPPENDSTRINGVAR_MAXLENGTH));
  if (jsvGetStringLength(v)==0) jsvAppendString(v, "/");
  jsvUnLock(jsvObjectSetChild(obj, "pathname", v));

  jsvUnLock(jsvObjectSetChild(obj, "search", (searchStart>=0)?jsvNewFromStringVar(url, (size_t)searchStart, JSVAPPENDSTRINGVAR_MAXLENGTH):jsvNewNull()));

  if (portNumber<=0 || portNumber>65535) portNumber=80;
  jsvUnLock(jsvObjectSetChild(obj, "port", jsvNewFromInteger(portNumber)));

  JsVar *query = (searchStart>=0)?jsvNewFromStringVar(url, (size_t)(searchStart+1), JSVAPPENDSTRINGVAR_MAXLENGTH):jsvNewNull();
  if (parseQuery && !jsvIsNull(query)) {
    JsVar *queryStr = query;
    jsvStringIteratorNew(&it, query, 0);
    query = jsvNewWithFlags(JSV_OBJECT);

    JsVar *key = jsvNewFromEmptyString();
    JsVar *val = jsvNewFromEmptyString();
    bool hadEquals = false;

    while (jsvStringIteratorHasChar(&it)) {
      char ch = jsvStringIteratorGetChar(&it);
      if (ch=='&') {
        if (jsvGetStringLength(key)>0 || jsvGetStringLength(val)>0) {
          key = jsvAsArrayIndexAndUnLock(key); // make sure "0" gets made into 0
          jsvMakeIntoVariableName(key, val);
          jsvAddName(query, key);
          jsvUnLock(key);
          jsvUnLock(val);
          key = jsvNewFromEmptyString();
          val = jsvNewFromEmptyString();
          hadEquals = false;
        }
      } else if (!hadEquals && ch=='=') {
        hadEquals = true;
      } else {
        // decode percent escape chars
        if (ch=='%') {
          jsvStringIteratorNext(&it);
          ch = jsvStringIteratorGetChar(&it);
          jsvStringIteratorNext(&it);
          ch = (char)((chtod(ch)<<4) | chtod(jsvStringIteratorGetChar(&it)));
        }

        if (hadEquals) jsvAppendCharacter(val, ch);
        else jsvAppendCharacter(key, ch);
      }
      jsvStringIteratorNext(&it);
      charIdx++;
    }
    jsvStringIteratorFree(&it);
    jsvUnLock(queryStr);

    if (jsvGetStringLength(key)>0 || jsvGetStringLength(val)>0) {
      key = jsvAsArrayIndexAndUnLock(key); // make sure "0" gets made into 0
      jsvMakeIntoVariableName(key, val);
      jsvAddName(query, key);
    }
    jsvUnLock(key);
    jsvUnLock(val);
  }
  jsvUnLock(jsvObjectSetChild(obj, "query", query));

  return obj;
}


// ---------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------


/*JSON{
  "type" : "library",
  "class" : "net"
}
This library allows you to create TCPIP servers and clients

In order to use this, you will need an extra module to get network connectivity.

This is designed to be a cut-down version of the [node.js library](http://nodejs.org/api/net.html). Please see the [Internet](/Internet) page for more information on how to use it.
*/

/*JSON{
  "type" : "class",
  "library" : "net",
  "class" : "Server"
}
The socket server created by `require('net').createServer`
*/
/*JSON{
  "type" : "class",
  "library" : "net",
  "class" : "Socket"
}
An actual socket connection - allowing transmit/receive of TCP data
*/
/*JSON{
  "type" : "event",
  "class" : "Socket",
  "name" : "data",
  "params" : [
    ["data","JsVar","A string containing one or more characters of received data"]
  ]
}
The 'data' event is called when data is received. If a handler is defined with `X.on('data', function(data) { ... })` then it will be called, otherwise data will be stored in an internal buffer, where it can be retrieved with `X.read()`
*/
/*JSON{
  "type" : "event",
  "class" : "Socket",
  "name" : "close"
}
Called when the connection closes.
*/
/*JSON{
  "type" : "method",
  "class" : "Socket",
  "name" : "available",
  "generate" : "jswrap_stream_available",
  "return" : ["int","How many bytes are available"]
}
Return how many bytes are available to read. If there is already a listener for data, this will always return 0.
*/
/*JSON{
  "type" : "method",
  "class" : "Socket",
  "name" : "read",
  "generate" : "jswrap_stream_read",
  "params" : [
    ["chars","int","The number of characters to read, or undefined/0 for all available"]
  ],
  "return" : ["JsVar","A string containing the required bytes."]
}
Return a string containing characters that have been received
*/
/*JSON{
  "type" : "method",
  "class" : "Socket",
  "name" : "pipe",
  "ifndef" : "SAVE_ON_FLASH",
  "generate" : "jswrap_pipe",
  "params" : [
    ["destination","JsVar","The destination file/stream that will receive content from the source."],
    ["options","JsVar",["An optional object `{ chunkSize : int=32, end : bool=true, complete : function }`","chunkSize : The amount of data to pipe from source to destination at a time","complete : a function to call when the pipe activity is complete","end : call the 'end' function on the destination when the source is finished"]]
  ]
}
Pipe this to a stream (an object with a 'write' method)
*/
/*JSON{
  "type" : "event",
  "class" : "Socket",
  "name" : "drain"
}
An event that is fired when the buffer is empty and it can accept more data to send. 
*/



// ---------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------
/*JSON{
  "type" : "staticmethod",
  "class" : "net",
  "name" : "createServer",
  "generate" : "jswrap_net_createServer",
  "params" : [
    ["callback","JsVar","A `function(connection)` that will be called when a connection is made"]
  ],
  "return" : ["JsVar","Returns a new Server Object"],
  "return_object" : "Server"
}
Create a Server

When a request to the server is made, the callback is called. In the callback you can use the methods on the connection to send data. You can also add `connection.on('data',function() { ... })` to listen for received data
*/

JsVar *jswrap_net_createServer(JsVar *callback) {
  JsVar *skippedCallback = jsvSkipName(callback);
  if (!jsvIsFunction(skippedCallback)) {
    jsError("Expecting Callback Function but got %t", skippedCallback);
    jsvUnLock(skippedCallback);
    return 0;
  }
  jsvUnLock(skippedCallback);
  return serverNew(ST_NORMAL, callback);
}



/*JSON{
  "type" : "staticmethod",
  "class" : "net",
  "name" : "connect",
  "generate_full" : "jswrap_net_connect(options, callback, ST_NORMAL)",
  "params" : [
    ["options","JsVar","An object containing host,port fields"],
    ["callback","JsVar","A function(res) that will be called when a connection is made. You can then call `res.on('data', function(data) { ... })` and `res.on('close', function() { ... })` to deal with the response."]
  ],
  "return" : ["JsVar","Returns a new net.Socket object"],
  "return_object" : "Socket"
}
Create a socket connection
*/
JsVar *jswrap_net_connect(JsVar *options, JsVar *callback, SocketType socketType) {
  bool unlockOptions = false;
  if (jsvIsString(options)) {
    options = jswrap_url_parse(options, false);
    unlockOptions = true;
  }
  if (!jsvIsObject(options)) {
    jsError("Expecting Options to be an Object but it was %t", options);
    return 0;
  }
  JsVar *skippedCallback = jsvSkipName(callback);
  if (!jsvIsFunction(skippedCallback)) {
    jsError("Expecting Callback Function but got %t", skippedCallback);
    jsvUnLock(skippedCallback);
    return 0;
  }
  jsvUnLock(skippedCallback);
  JsVar *rq = clientRequestNew(socketType, options, callback);
  if (unlockOptions) jsvUnLock(options);

  if (socketType!=ST_HTTP) {
    JsNetwork net;
    if (networkGetFromVarIfOnline(&net)) {
      clientRequestConnect(&net, rq);
    }
    networkFree(&net);
  }

  return rq;
}


// ---------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------

/*JSON{
  "type" : "method",
  "class" : "Server",
  "name" : "listen",
  "generate" : "jswrap_net_server_listen",
  "params" : [
    ["port","int32","The port to listen on"]
  ]
}
Start listening for new connections on the given port
*/

void jswrap_net_server_listen(JsVar *parent, int port) {
  JsNetwork net;
  if (!networkGetFromVarIfOnline(&net)) return;

  serverListen(&net, parent, port);
  networkFree(&net);
}

/*JSON{
  "type" : "method",
  "class" : "Server",
  "name" : "close",
  "generate" : "jswrap_net_server_close"
}
Stop listening for new connections
*/

void jswrap_net_server_close(JsVar *parent) {
  JsNetwork net;
  if (!networkGetFromVarIfOnline(&net)) return;

  serverClose(&net, parent);
  networkFree(&net);
}

// ---------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------
/*JSON{
  "type" : "method",
  "class" : "Socket",
  "name" : "write",
  "generate" : "jswrap_net_socket_write",
  "params" : [
    ["data","JsVar","A string containing data to send"]
  ],
  "return" : ["bool","For note compatibility, the boolean false. When the send buffer is empty, a `drain` event will be sent"]
}*/
bool jswrap_net_socket_write(JsVar *parent, JsVar *data) {
  clientRequestWrite(parent, data);
  return false;
}

/*JSON{
  "type" : "method",
  "class" : "Socket",
  "name" : "end",
  "generate" : "jswrap_net_socket_end",
  "params" : [
    ["data","JsVar","A string containing data to send"]
  ]
}
Close this socket - optional data to append as an argument
*/
void jswrap_net_socket_end(JsVar *parent, JsVar *data) {
  JsNetwork net;
  if (!networkGetFromVarIfOnline(&net)) return;

  if (!jsvIsUndefined(data)) jswrap_net_socket_write(parent, data);
  clientRequestEnd(&net, parent);
  networkFree(&net);
}
