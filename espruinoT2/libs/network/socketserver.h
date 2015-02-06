/*
 * This file is part of Espruino, a JavaScript interpreter for Microcontrollers
 *
 * Copyright (C) 2013 Gordon Williams <gw@pur3.co.uk>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * ----------------------------------------------------------------------------
 * Contains HTTP client and server
 * ----------------------------------------------------------------------------
 */
#ifndef SOCKETSERVER_H
#define SOCKETSERVER_H


#include "jsutils.h"
#include "jsvar.h"
#include "../network.h"

typedef enum {
  ST_NORMAL, // standard socket client/server
  ST_HTTP, // HTTP client/server
  // WebSockets?
} SocketType;


// -----------------------------
void socketInit();
void socketKill(JsNetwork *net);
bool socketIdle(JsNetwork *net);

// -----------------------------
JsVar *serverNew(SocketType socketType, JsVar *callback);
void serverListen(JsNetwork *net, JsVar *httpServerVar, int port);
void serverClose(JsNetwork *net, JsVar *server);

JsVar *clientRequestNew(SocketType socketType, JsVar *options, JsVar *callback);
void clientRequestWrite(JsVar *httpClientReqVar, JsVar *data);
void clientRequestConnect(JsNetwork *net, JsVar *httpClientReqVar);
void clientRequestEnd(JsNetwork *net, JsVar *httpClientReqVar);

void serverResponseWriteHead(JsVar *httpServerResponseVar, int statusCode, JsVar *headers);
void serverResponseData(JsVar *httpServerResponseVar, JsVar *data);
void serverResponseEnd(JsVar *httpServerResponseVar);

#endif // SOCKETSERVER_H
