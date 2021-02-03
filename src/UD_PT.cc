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
//  File:               UD_PT.cc
//  Description:        UD test port source
//  Rev:                R2A
//  Prodnr:             CNL 113 702
//


#include "UD_PT.hh"
#include "UD_PortType.hh"
#include "UD_Types.hh"

#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <memory.h>
#include <iostream>

#define DEFAULT_LOCAL_PORT	(50000)
#define DEFAULT_NUM_CONN	(10)

using namespace UD__Types;

namespace UD__PortType {

INTEGER simpleGetMsgLen(const OCTETSTRING& stream, ro__integer& /*args*/)
{
	return stream.lengthof();
}

UD__PT_PROVIDER::UD__PT_PROVIDER(const char *par_port_name)
: PORT(par_port_name)
, debugging(false)
, sock_type(SOCK_STREAM)
, target_fd(-1)
{
	conn_list = NULL;
	num_of_conn  = 0;
	conn_list_length = 0;
	operation_mode=false;

	conn_list_server = NULL;
	num_of_conn_server  = 0;
	conn_list_length_server = 0;
	defaultGetMsgLen = simpleGetMsgLen;
	defaultMsgLenArgs = new ro__integer(NULL_VALUE);
}

UD__PT_PROVIDER::~UD__PT_PROVIDER()
{
	Free(conn_list);
	Free(conn_list_server);
}

void UD__PT_PROVIDER::setUpSocket()
{
	log("entering UD__PT::setUpSocket()");
	log("leaving UD__PT::setUpSocket()");
}

void UD__PT_PROVIDER::closeDownSocket()
{
	log("entering UD__PT::closeDownSocket()");

	for(int a=0;a<conn_list_length_server;a++){
		if(conn_list_server[a].status){
			close(conn_list_server[a].fd);
			//close(conn_list_server[a].conn);

			Handler_Remove_Fd_Read(conn_list_server[a].fd);
			//Handler_Remove_Fd_Read(conn_list_server[a].conn);

			unlink(conn_list_server[a].remote_Addr.sun_path);
		}
	}

	Free(conn_list_server);
	conn_list_server = NULL;

	for(int a=0;a<conn_list_length;a++){
		if(conn_list[a].status){
			close(conn_list[a].fd);
			Handler_Remove_Fd_Read(conn_list[a].fd);
		}

	}

	Free(conn_list);
	conn_list = NULL;

	log("entering UD__PT::closeDownSocket()");
}

void UD__PT_PROVIDER::log(const char *fmt, ...)
{
	if (debugging) {
		TTCN_Logger::begin_event(TTCN_DEBUG);
		TTCN_Logger::log_event("UD test port (%s): ", get_name());
		va_list args;
		va_start(args, fmt);
		TTCN_Logger::log_event_va_list(fmt, args);
		va_end(args);
		TTCN_Logger::end_event();
	}
}

void UD__PT_PROVIDER::logHex(const char *prompt, const OCTETSTRING& msg)
{
	if (debugging) { //if debug
		TTCN_Logger::begin_event(TTCN_DEBUG);
		TTCN_Logger::log_event_str(prompt);
		TTCN_Logger::log_event("Size: %d,\nMsg: ",msg.lengthof());

		for(int i=0; i<msg.lengthof(); i++)
			TTCN_Logger::log_event(" %02x", ((const unsigned char*)msg)[i]);
		TTCN_Logger::log_event("\n");
		TTCN_Logger::end_event();
	}
}

void UD__PT_PROVIDER::set_parameter(const char *parameter_name,
		const char *parameter_value)
{
	log("entering UD__PT::set_parameter(%s, %s)", parameter_name, parameter_value);

	if (!strcmp(parameter_name, "socket_type")) {
#ifdef SOCK_SEQPACKET
		if (!strcasecmp(parameter_value, "SEQPACKET"))
			sock_type = SOCK_SEQPACKET;
		else
#endif
		if (!strcasecmp(parameter_value, "STREAM"))
			sock_type = SOCK_STREAM;
		else if (!strcasecmp(parameter_value, "DGRAM"))
			sock_type = SOCK_DGRAM;
		else
			TTCN_warning("UD__PT_PROVIDER::set_parameter: %s: Invalid %s value '%s'", port_name, parameter_name, parameter_value);
	}

	log("leaving UD__PT::set_parameter(%s, %s)", parameter_name, parameter_value);
}

void UD__PT_PROVIDER::Handle_Fd_Event_Error(int fd)
{
}

void UD__PT_PROVIDER::Handle_Fd_Event_Writable(int fd)
{
}

void UD__PT_PROVIDER::Handle_Fd_Event_Readable(int fd)
{
	log("entering UD__PT::Handle_Fd_Event_Readable()");
	unsigned char msg[65535];        // Allocate memory for possible messages

	//UD__Types::UD__send__data__s parameters_s;
	UD__Types::UD__send__data parameters;

			for(int a=0;a<conn_list_length;a++){

			if(fd == conn_list[a].fd){

				if(conn_list[a].status){
					int size_read = recv(conn_list[a].fd, msg, sizeof(msg), 0);

					if (size_read <= 0){
						//there is an empty message, or error in receive
						//remove the socket
						int close_fd=conn_list[a].fd;
						std::cout << "close_fd" << close_fd << std::endl;
						conn_list[a].status=0;
						Free(conn_list[a].buf);
						conn_list[a].buf = NULL;
						num_of_conn--;
						Handler_Remove_Fd_Read(close_fd);
						close(close_fd);

						//unlink(conn_list[a].remote_Addr.sun_path);
					}
					else{
						if (sock_type == SOCK_STREAM) {
							/* append just-read data to the buffer */
							(*conn_list[a].buf)->put_s(size_read, msg);

							bool msgFound = false;
							do {
								/* determine message length by callback function */
								if (conn_list[a].getMsgLen != simpleGetMsgLen) {
									if (conn_list[a].msgLen == -1) {
										OCTETSTRING oct;
										(*conn_list[a].buf)->get_string(oct);
										conn_list[a].msgLen = conn_list[a].getMsgLen.invoke(oct, *conn_list[a].msgLenArgs);
									}
								} else {
									conn_list[a].msgLen = (*conn_list[a].buf)->get_len();
								}
								/* did we find a complete message inside the buffer? */
								msgFound = (conn_list[a].msgLen != -1) && (conn_list[a].msgLen <= (int)conn_list[a].buf[0]->get_len());
								if (msgFound) {
									parameters.data() = OCTETSTRING(conn_list[a].msgLen, conn_list[a].buf[0]->get_data());
									conn_list[a].buf[0]->set_pos((size_t)conn_list[a].msgLen);
									conn_list[a].buf[0]->cut();
									parameters.id() = a;
									incoming_message(parameters);
									conn_list[a].msgLen = -1;
								}
								/* continue to iterate as long as data is left and callback continues to find full messages inside */
							} while (msgFound && conn_list[a].buf[0]->get_len() != 0);
						} else {
							/* SOCK_DGRAM, SOCK_SEQPACKET: forward directly to
							 * testsuite, bypassing buffering + getMsgLen() */
							parameters.data() = OCTETSTRING(size_read, msg);
							parameters.id() = a;
							incoming_message(parameters);
						}
					}
				}
			}
		}




		for(int a=0;a<conn_list_length_server;a++){

			if(conn_list_server[a].status){

				if(fd == conn_list_server[a].fd){

					struct sockaddr_un store_addr;

					strcpy(store_addr.sun_path, conn_list_server[a].remote_Addr.sun_path);

					socklen_t addr_length = sizeof(conn_list_server[a].remote_Addr);

					int cn=0;
					if(num_of_conn<conn_list_length){
						cn=0;

						while(conn_list[cn].status)
						{
							cn++;
						}
					}
					UD__Types::UD__connected result;

					if(
							(conn_list[cn].fd = accept(conn_list_server[a].fd, (struct sockaddr*)&conn_list_server[a].remote_Addr, &addr_length))
							>= 0 ){
						conn_list[cn].status = 1;

						num_of_conn++;

						strcpy(conn_list_server[a].remote_Addr.sun_path, store_addr.sun_path);
						strcpy(conn_list[cn].remote_Addr.sun_path, store_addr.sun_path);

						result.result()().result__code() = UD__Types::UD__Result__code::SUCCESS;
						result.result()().err() = OMIT_VALUE;

					}
					else{
						result.result()().result__code() = UD__Types::UD__Result__code::ERROR_;
						result.result()().err() = "Accept rejected";
					}

					result.id()=cn;
					result.path() = conn_list[cn].remote_Addr.sun_path;

					incoming_message(result);

					Handler_Add_Fd_Read(conn_list[cn].fd);
				}
			}
		}




	//}
	log("leaving UD__PT::Handle_Fd_Event_Readable()");
}

void UD__PT_PROVIDER::user_map(const char *system_port)
{
	log("entering UD__PT::user_map()");

	if (conn_list != NULL) TTCN_error("UD Test Port (%s): Internal error: "
			"conn_list is not NULL when mapping.", port_name);
	conn_list = (conn_data*)Malloc(DEFAULT_NUM_CONN * sizeof(*conn_list));
	num_of_conn=0;
	conn_list_length=DEFAULT_NUM_CONN;
	for(int a=0;a<conn_list_length;a++){conn_list[a].status=0;}

	if (conn_list_server != NULL) TTCN_error("UD Test Port (%s): Internal error: "
			"conn_list_server is not NULL when mapping.", port_name);
	conn_list_server = (conn_data_server*)Malloc(DEFAULT_NUM_CONN * sizeof(*conn_list_server));
	num_of_conn_server=0;
	conn_list_length_server=DEFAULT_NUM_CONN;
	for(int a=0;a<conn_list_length_server;a++){conn_list_server[a].status=0;}

	log("leaving UD__PT::user_map()");
}

void UD__PT_PROVIDER::user_unmap(const char *system_port)
{
	log("entering UD__PT::user_unmap()");

	closeDownSocket();

	log("leaving UD__PT::user_unmap()");
}

void UD__PT_PROVIDER::user_start()
{
}

void UD__PT_PROVIDER::user_stop()
{
}



void UD__PT_PROVIDER::outgoing_send(const UD__Types::UD__close& send_par)
{
	  log("entering UD__PT::outgoing_send(UD__close)");
	  int close_fd=conn_list[send_par.id()].fd;

	  conn_list[send_par.id()].status=0;
	  conn_list[send_par.id()].fd=0;
	  num_of_conn--;
	  Handler_Remove_Fd_Read(close_fd);

	  close(close_fd);

	  log("leaving UD__PT::outgoing_send(UD__close)");
}

void UD__PT_PROVIDER::outgoing_send(const UD__Types::UD__connect& send_par)
{
	//Client connects to certain server
	log("entering UD__PT::outgoing_send(UD__connect)");
	int cn;
	UD__Types::UD__connect__result result;
	struct sockaddr_un localAddr;
	localAddr.sun_family = AF_UNIX;

	strcpy(serveraddr.sun_path,send_par.path());

	if(num_of_conn<conn_list_length){
		cn=0;

		while(conn_list[cn].status)
		{
			cn++;
		}
	}
	else{
		conn_list = (conn_data*)Realloc(conn_list, 2 * conn_list_length * sizeof(*conn_list));
		for(int a=conn_list_length;a<conn_list_length*2;a++){conn_list[a].status=0;}
		cn=conn_list_length;
		conn_list_length*=2;
	}

	if((target_fd = socket(PF_UNIX,sock_type,0))<0) {
		TTCN_error("Cannot open socket \n");
	}

	size_t serveraddrLength;
	serveraddrLength = sizeof(serveraddr.sun_family) + strlen(serveraddr.sun_path);
	serveraddr.sun_family = AF_UNIX;

	if(connect(target_fd, (struct sockaddr *) &serveraddr, serveraddrLength) != 0)
	//if(connect(send_par.id(), (struct sockaddr *) &serveraddr, serveraddrLength) != 0)
	{

		result.result()().result__code() = UD__Types::UD__Result__code::ERROR_;
		result.result()().err() = "Connect was unsuccessful";

	}
	else{
		result.result()().result__code() = UD__Types::UD__Result__code::SUCCESS;
		result.result()().err() = OMIT_VALUE;

	}

	conn_list[cn].fd=target_fd;
	conn_list[cn].status=1;
	conn_list[cn].remote_Addr.sun_family = AF_UNIX;
	conn_list[cn].getMsgLen = defaultGetMsgLen;
	conn_list[cn].msgLenArgs = new ro__integer(*defaultMsgLenArgs);
	if (sock_type == SOCK_STREAM) {
		conn_list[cn].buf = (TTCN_Buffer **)Malloc(sizeof(TTCN_Buffer *));
		*conn_list[cn].buf = new TTCN_Buffer;
		conn_list[cn].msgLen = -1;
	} else
		conn_list[cn].buf = NULL;

	strcpy(conn_list[cn].remote_Addr.sun_path,send_par.path());

	num_of_conn++;

	Handler_Add_Fd_Read(target_fd);


	result.id()=cn;
	//result.path() = send_par.path();

	incoming_message(result);
	log("leaving UD__PT::outgoing_send(UD__connect)");


}

void UD__PT_PROVIDER::outgoing_send(const UD__Types::UD__listen& send_par)
{
	log("entering UD__PT::outgoing_send(UD__listen)");
	int cn;
	UD__Types::UD__listen__result result;
	struct sockaddr_un localAddr;
	localAddr.sun_family = AF_UNIX;

	strcpy(serveraddr.sun_path,send_par.path());

	if(num_of_conn_server<conn_list_length_server){
		cn=0;
		while(conn_list_server[cn].status){cn++;}
	}
	else{
		conn_list_server = (conn_data_server*)Realloc(conn_list_server, 2 * conn_list_length_server * sizeof(*conn_list_server));
		for(int a=conn_list_length_server;a<conn_list_length_server*2;a++){conn_list_server[a].status=0;}
		cn=conn_list_length_server;
		conn_list_length_server*=2;
	}

	if((target_fd = socket(PF_UNIX,sock_type,0))<0) {
		TTCN_warning("Cannot open socket \n");

	}


	unlink(serveraddr.sun_path);
	size_t serveraddrLength;
	serveraddr.sun_family = AF_UNIX;
	serveraddrLength = sizeof(serveraddr.sun_family) + strlen(serveraddr.sun_path);
	if(bind(target_fd, (struct sockaddr *) &serveraddr, serveraddrLength )<0) {
		close(target_fd);
		TTCN_warning("Cannot bind port\n");
		result.result()().result__code() = UD__Types::UD__Result__code::ERROR_;

	}

	if(listen(target_fd, 5)){
		TTCN_warning("Error while listening");
		result.result()().result__code() = UD__Types::UD__Result__code::ERROR_;
	}

	//conn_list_server[cn].fd=sock;
	conn_list_server[cn].fd=target_fd;
	conn_list_server[cn].status=1;
	conn_list_server[cn].remote_Addr.sun_family = AF_UNIX;

	strcpy(conn_list_server[cn].remote_Addr.sun_path,send_par.path());

	num_of_conn_server++;


	Handler_Add_Fd_Read(target_fd);

	//UD__Types::UD__open__result result;

	result.id()=cn;
	result.result()().result__code() = UD__Types::UD__Result__code::SUCCESS;
	result.result()().err() = OMIT_VALUE;

	incoming_message(result);
	log("leaving UD__PT::outgoing_send(UD__listen)");
}

void UD__PT_PROVIDER::outgoing_send(const UD__Types::UD__shutdown& send_par)
{
	log("entering UD__PT::outgoing_send(UD__Shutdown)");

	/**if(num_of_conn > 0){

		if(conn_list[send_par.id()].status!=0){
			int close_fd=conn_list[send_par.id()].fd;
			conn_list[send_par.id()].status=0;
			num_of_conn--;
			Handler_Remove_Fd_Read(close_fd);

			close(close_fd);

			unlink(conn_list[send_par.id()].remote_Addr.sun_path);
		}
		else{
			//it has been already removed
		}
	}*/

	if(conn_list_server[send_par.id()].status!=0){
			int close_fd=conn_list_server[send_par.id()].fd;
			conn_list_server[send_par.id()].status=0;
			num_of_conn_server--;
			Handler_Remove_Fd_Read(close_fd);

			close(close_fd);

			unlink(conn_list_server[send_par.id()].remote_Addr.sun_path);
	}



	log("leaving UD__PT::outgoing_send(UD__Shutdown)");
}

void UD__PT_PROVIDER::outgoing_send(const UD__Types::UD__send__data& send_par)
{
	log("entering UD__PT::outgoing_send(UD__send__data)");
	logHex("Sending data: ", send_par.data());
	int sock;
	struct sockaddr_un targetAddr;
	targetAddr.sun_family = AF_UNIX;


	int cn=send_par.id();

	//targetAddr.sun_path = conn_list[cn].remote_Addr.sun_path;
	strcpy(targetAddr.sun_path,conn_list[cn].remote_Addr.sun_path);

	sock=conn_list[cn].fd;

	int nbytes = send_par.data().lengthof();

	int nrOfBytesSent;
	//if ((nrOfBytesSent = write(sock, send_par.data(), nbytes) ) != nbytes)
	if ((nrOfBytesSent = send(sock, send_par.data(), nbytes, 0) ) != nbytes)
	{
		TTCN_error("Write error");
	}

	log("Nr of bytes sent = %d", nrOfBytesSent);

	if (nrOfBytesSent != send_par.data().lengthof()) {
		TTCN_error("Sendto system call failed: %d bytes was sent instead of %d", nrOfBytesSent, send_par.data().lengthof());
	}

	log("leaving UDPasp__PT::outgoing_send(ASP__UDP__message)");

}

void f__UD__PT_PROVIDER__setGetMsgLen(UD__PT_PROVIDER& portRef, const INTEGER& connId, f__UD__getMsgLen& f, const ro__integer& msgLenArgs)
{
	if ((int)connId == -1) {
		portRef.defaultGetMsgLen = f;
		delete portRef.defaultMsgLenArgs;
		portRef.defaultMsgLenArgs = new Socket__API__Definitions::ro__integer(msgLenArgs);
	} else {
		if (!portRef.isConnIdValid(connId)) {
			TTCN_error("UD: cannot setGetMsgLen: connId %d not valid", (int)connId);
			return;
		}
		portRef.conn_list[(int)connId].getMsgLen = f;
		delete portRef.conn_list[connId].msgLenArgs;
		portRef.conn_list[connId].msgLenArgs = new Socket__API__Definitions::ro__integer(msgLenArgs);
	}
}

void f__UD__setGetMsgLen(UD__PT& portRef, const INTEGER& connId, f__UD__getMsgLen& f, const ro__integer& msgLenArgs) {
	f__UD__PT_PROVIDER__setGetMsgLen(portRef, connId, f, msgLenArgs);
}


}
