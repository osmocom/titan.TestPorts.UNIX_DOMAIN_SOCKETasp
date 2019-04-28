/******************************************************************************
* Copyright (c) 2000-2019 Ericsson Telecom AB
* All rights reserved. This program and the accompanying materials
* are made available under the terms of the Eclipse Public License v2.0
* which accompanies this distribution, and is available at
* https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html
*
* Contributors:
* Krisztian Pandi
******************************************************************************/
//
//  File:               UD_PT.hh
//  Description:        UD test port header
//  Rev:                R2A
//  Prodnr:             CNL 113 702
//


#ifndef UD__PT_HH
#define UD__PT_HH

#include <TTCN3.hh>
#include "UD_Types.hh"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace UD__Types {
  class UD;
}

namespace UD__PortType {

class UD__PT_PROVIDER : public PORT {
public:
  UD__PT_PROVIDER(const char *par_port_name=NULL);
  ~UD__PT_PROVIDER();

  void set_parameter(const char *parameter_name,
    const char *parameter_value);

  struct conn_data{
    int fd;
    //int port_num;
    int status;
    //struct sockaddr_in remote_Addr;
    struct sockaddr_un remote_Addr;
    int msgLen; // -1 or the message length returned by getMsgLen
    TTCN_Buffer **buf;
    Socket__API__Definitions::f__getMsgLen getMsgLen;
    Socket__API__Definitions::ro__integer *msgLenArgs;
  };

  conn_data *conn_list;

  inline bool isConnIdValid(int connId) const {
    return (connId < conn_list_length && connId >= 0 &&
        conn_list != NULL && conn_list[connId].fd > 0);
  }

  /**void Event_Handler(const fd_set *read_fds,
    const fd_set *write_fds, const fd_set *error_fds,
    double time_since_last_call);*/
private:
  void Handle_Fd_Event_Error(int fd);
  void Handle_Fd_Event_Writable(int fd);
  void Handle_Fd_Event_Readable(int fd);

protected:
  void user_map(const char *system_port);
  void user_unmap(const char *system_port);

  void user_start();
  void user_stop();

  void outgoing_send(const UD__Types::UD__close& send_par);

  void outgoing_send(const UD__Types::UD__connect& send_par);
  void outgoing_send(const UD__Types::UD__listen& send_par);
  void outgoing_send(const UD__Types::UD__shutdown& send_par);
  void outgoing_send(const UD__Types::UD__send__data& send_par);
  
  virtual void incoming_message(const UD__Types::UD__connect__result& incoming_par) = 0;
  virtual void incoming_message(const UD__Types::UD__listen__result& incoming_par) = 0;
  virtual void incoming_message(const UD__Types::UD__connected& incoming_par) = 0;

  virtual void incoming_message(const UD__Types::UD__send__data& incoming_par) = 0;

  void log(const char *fmt, ...);
  void logHex(const char *prompt, const OCTETSTRING& msg);
  void setUpSocket();
  void closeDownSocket();

private:
  bool debugging;
  bool operation_mode;  // false: basic mode. One socket.
                   // true: advanced mode. The new features are activated
  bool broadcast;
  int sock_type;
  Socket__API__Definitions::f__getMsgLen defaultGetMsgLen;
  Socket__API__Definitions::ro__integer *defaultMsgLenArgs;
  
  struct conn_data_server{
      int fd;
      int status;
      struct sockaddr_un remote_Addr;
      int conn;
    };

  conn_data_server *conn_list_server;

  int num_of_conn;
  int num_of_conn_server;
  int conn_list_length;
  int conn_list_length_server;
  struct sockaddr_in localAddr;
  int target_fd;
  fd_set conn_map;
  fd_set conn_map_server;
  int conn;
  ///
  struct sockaddr_un serveraddr;

public:
  friend void f__UD__PT_PROVIDER__setGetMsgLen(
			UD__PT_PROVIDER& portRef,
			const INTEGER& connId,
			Socket__API__Definitions::f__getMsgLen& f,
			const Socket__API__Definitions::ro__integer& msgLenArgs);

};
}
#endif
