#include "net.h"

#include <jsbx/event.h>
#include <jsbx/api.h>
#include <jerryscript.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>


#include <list>
#include <string>
#include <fstream>
#include <streambuf>
#include <list>
#include <map>
#include <algorithm>



class TCPClient
{
public:
  TCPClient()
    : mTCPClientObject(0)
    , sock(0)
  {}

  bool operator ==(const TCPClient &other)
  {
    return (sock == other.sock);
  }

  bool operator ==(const int otherSock)
  {
    return (sock == otherSock);
  }

  jerry_value_t mTCPClientObject;
  int sock;
};


typedef std::list<TCPClient> SocketList;
typedef SocketList::iterator SocketListIterator;





class TCPServer
{
public:
  TCPServer(jerry_value_t TCPServerObject, int sock)
    : mTCPServerObject(TCPServerObject)
    , mSock(sock)
    , mHasPendingConnections(false)
  {}

  bool operator ==(const TCPServer &other)
  {//other.mTCPServerObject == mTCPServerObject ||
    return (other.mSock == mSock);
  }

//private:
  jerry_value_t mTCPServerObject;
  int mSock;
  SocketList mClientList;
  bool mHasPendingConnections;
};



typedef std::list<TCPServer> TCPServerList;
typedef TCPServerList::iterator TCPServerListIterator;

TCPServerList tcpServerList;
std::list<int /* client socket */> readyReadSockets; // TODO: move into eventList
SocketList pendingConnects; // TODO: move into eventList, BUT don't remove event while checking......
SocketList freeStandingTCPClients;













class FailedConnectEvent : public AbstractScriptEvent
{
public:
  FailedConnectEvent(TCPClient &tcpClient)
    : AbstractScriptEvent(AbstractScriptEvent::Type::FailedConnectEvent)
    , mTcpCLient(tcpClient)
  {}

  TCPClient mTcpCLient;
};



class ConnectedEvent : public AbstractScriptEvent
{
public:
  ConnectedEvent(TCPClient &tcpClient)
    : AbstractScriptEvent(AbstractScriptEvent::Type::ConnectedEvent)
    , mTcpCLient(tcpClient)
  {}

  TCPClient mTcpCLient;
};














class DiscoEvent : public AbstractWorkerEvent
{
public:
  //DiscoEvent(TCPClient &tcpClient)
  DiscoEvent(int sock)
    : AbstractWorkerEvent(AbstractWorkerEvent::Type::DiscoEvent)
    //, mTcpCLient(tcpClient)
    , mSock(sock)
  {}

  //TCPClient mTcpCLient;#
  int mSock;
};












































pthread_mutex_t *mutex1;









static jerry_value_t internal_listen_handler(const jerry_value_t function_obj,
                                             const jerry_value_t this_val,
                                             const jerry_value_t args[],
                                             const jerry_length_t argc)
{
  //puts("internal_listen_handler called");

  if (argc != 2)
  {
    puts("uieduiojefj");
    return jerry_create_boolean(false);
  }


  jerry_value_t socket_value = args[0];

  if (!jerry_value_is_object(socket_value))
    {
    puts("uiedusdsdsdsdsdsdiojefj");
    return jerry_create_boolean(false);
  }


  jerry_value_t queue_value = args[1];

  if (!jerry_value_is_number(queue_value))
  {
    puts("232323ui23eduio223jefj");
    return jerry_create_boolean(false);
  }

  uint16_t queue = jerry_get_number_value(queue_value);



  handle_container_t *handle_container = nullptr;
  bool has_file_handle = jerry_get_object_native_pointer(socket_value, (void**)&handle_container, 0);

  if (!has_file_handle || handle_container == nullptr)
  {
    puts("no valid socket handle");
    return jerry_create_boolean(false);
  }

  int sock = handle_container->handle;



  if (listen(sock, queue) < 0)
  {
    puts("!listen");
    return jerry_create_boolean(false);
  }


  return jerry_create_boolean(true);
}





static jerry_value_t internal_bind_handler(const jerry_value_t function_obj,
                                           const jerry_value_t this_val,
                                           const jerry_value_t args[],
                                           const jerry_length_t argc)
{
  //puts("internal_bind_handler called");

  if (argc != 2)
    return jerry_create_boolean(false);


  jerry_value_t socket_value = args[0];

  if (!jerry_value_is_object(socket_value))
    return jerry_create_boolean(false);


  jerry_value_t port_value = args[1];

  if (!jerry_value_is_number(port_value))
    return jerry_create_boolean(false);

  uint16_t port = jerry_get_number_value(port_value);


  struct sockaddr_in server;
  memset(&server, 0, sizeof(server));
  server.sin_family = AF_INET;
  //server.sin_addr.s_addr = htonl(...);
  server.sin_port = htons(port);



  handle_container_t *handle_container = nullptr;
  bool has_file_handle = jerry_get_object_native_pointer(socket_value, (void**)&handle_container, 0);

  if (!has_file_handle || handle_container == nullptr)
  {
    puts("no valid socket handle");
    return jerry_create_boolean(false);
  }

  int sock = handle_container->handle;



  if (bind(sock, (struct sockaddr*)&server, sizeof(server)) < 0)
  {
    puts("!bind");
    return jerry_create_boolean(false);
  }


  return jerry_create_boolean(true);
}






int efd = 0;
















static jerry_value_t internal_registerTCPServer_handler(const jerry_value_t function_obj,
                                                        const jerry_value_t this_val,
                                                        const jerry_value_t args[],
                                                        const jerry_length_t argc)
{
  //puts("internal_registerTCPServer_handler called");

  if (argc != 2)
    return jerry_create_number(0);


  jerry_value_t TCPServerObject_value = jerry_acquire_value(args[0]); // TODO: release value somewhere !! set native obj for TCPServer object and clean it there?

  if (!jerry_value_is_object(TCPServerObject_value))
  {
    puts("not an object..");
    return jerry_create_boolean(false);
  }



  jerry_value_t socket_value = args[1];

  // jerry_value_t prop_name = jerry_create_string((const jerry_char_t *)"socket");
  // jerry_value_t socket_value = jerry_get_property(this_val, prop_name);

  if (!jerry_value_is_object(socket_value))
  {
    puts("socket_value not an object :(");
    return jerry_create_boolean(false);
  }







  handle_container_t *handle_container = nullptr;
  bool has_file_handle = jerry_get_object_native_pointer(socket_value, (void**)&handle_container, 0);

  if (!has_file_handle || handle_container == nullptr)
  {
    puts("no valid socket handle");
    return jerry_create_boolean(false);
  }

  int sock = handle_container->handle;



  // ...
  // epoll for server socket

  {
    struct epoll_event event;

    event.data.fd = sock;
    event.events = EPOLLIN | EPOLLET;

    if (epoll_ctl(efd, EPOLL_CTL_ADD, sock, &event) < 0)
    {
      puts("!epoll_ctl");
      return jerry_create_boolean(false);
    }
  }



  pthread_mutex_lock(mutex1);

  printf("TCPServerObject_value = %d\n", TCPServerObject_value);
  printf("TCPServerObject_value is object? %s\n", jerry_value_is_object(TCPServerObject_value) ? "yes" : "no");

  tcpServerList.push_back(TCPServer(TCPServerObject_value, sock));

  pthread_mutex_unlock(mutex1);



  return jerry_create_boolean(true);
}




static jerry_value_t internal_socket_handler(const jerry_value_t function_obj,
                                             const jerry_value_t this_val,
                                             const jerry_value_t args_p[],
                                             const jerry_length_t args_cnt)
{
  //puts("internal_socket_handler called");

  if (args_cnt != 3)
    return jerry_create_number(0);


  jerry_value_t address_family_value = args_p[0];

  if (!jerry_value_is_number(address_family_value))
  {
    puts("internal_socket_handler: arg 0 - address_family_value not a number");
    return jerry_create_number(0);
  }


  jerry_value_t protocol_family_value = args_p[1];

  if (!jerry_value_is_number(protocol_family_value))
  {
    puts("internal_socket_handler: arg 1 - protocol_family_value not a number");
    return jerry_create_number(0);
  }


  jerry_value_t flags_value = args_p[2];

  if (!jerry_value_is_number(flags_value))
  {
    puts("internal_socket_handler: arg 2 - flags_value not a number");
    return jerry_create_number(0);
  }



  int address_family = jerry_get_number_value(address_family_value);
  int protocol_family = jerry_get_number_value(protocol_family_value);
  int flags = jerry_get_number_value(flags_value);



  //int cli = socket(AF_INET /* 2 */, SOCK_STREAM /* 1 */, 0);
  int sock = socket(address_family, protocol_family, flags);

  if (sock < 1)
    return jerry_create_number(0);



  // set non-blocking !! TODO: don't do that, outsource to extra call
  int currentFlags = fcntl(sock, F_GETFL, 0);
  if (currentFlags == -1)
    return jerry_create_number(0);

  currentFlags = currentFlags | O_NONBLOCK;

  if (fcntl(sock, F_SETFL, currentFlags) != 0)
    return jerry_create_number(0);




  {
    int optval = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
  }


  //printf("new socket: %d\n", sock);



  jerry_value_t handle_object = jerry_create_object();
  static const jerry_object_native_info_t native_obj_type_info =
  {
    .free_cb = native_close_cb
  };


  handle_container_t *handle_container = (handle_container_t *)malloc(sizeof(handle_container_t));
  handle_container->handle = sock;

  jerry_set_object_native_pointer(handle_object, handle_container, &native_obj_type_info);



  //puts("internal_socket_handler: done");
  return handle_object;
}










// (TCPServer, TCPClient)  Registers TCPClient to TCPServer
static jerry_value_t internal_registerTCPClient_handler(const jerry_value_t function_obj,
                                             const jerry_value_t this_val,
                                             const jerry_value_t args[],
                                             const jerry_length_t argc)
{
  //puts("internal_registerTCPClient_handler called");

  if (argc != 2)
  {
    puts("uieduiojefj");
    return jerry_create_boolean(false);
  }


  jerry_value_t TCPServer_socket_value = args[0];

  if (!jerry_value_is_object(TCPServer_socket_value))
  {
    puts("uieduiojefjsssssssssssssssss");
    return jerry_create_boolean(false);
  }


  jerry_value_t TCPClient_object_value = args[1];

  if (!jerry_value_is_object(TCPClient_object_value))
  {
    puts("hw98fhw9f8hw98hf");
    return jerry_create_boolean(false);
  }




  handle_container_t *handle_container1 = nullptr;
  bool has_file_handle1 = jerry_get_object_native_pointer(TCPServer_socket_value, (void**)&handle_container1, 0);

  if (!has_file_handle1 || handle_container1 == nullptr)
  {
    puts("no valid socket handle1");
    return jerry_create_boolean(false);
  }

  int TCPServer_sock = handle_container1->handle;





  jerry_value_t prop_name = jerry_create_string((const jerry_char_t *)"socket");
  jerry_value_t TCPClient_socket_value = jerry_get_property(TCPClient_object_value, prop_name);

  if (!jerry_value_is_object(TCPClient_socket_value))
  {
    puts("ooijoijoijojiojoijo");
    return jerry_create_boolean(false);
  }



  handle_container_t *handle_container2 = nullptr;
  bool has_file_handle2 = jerry_get_object_native_pointer(TCPClient_socket_value, (void**)&handle_container2, 0);

  if (!has_file_handle2 || handle_container2 == nullptr)
  {
    puts("no valid socket handle2");
    return jerry_create_boolean(false);
  }

  int TCPClient_sock = handle_container2->handle;
  TCPClient newTcpClient;
  printf("TCPClient_sock = %d\n", TCPClient_sock);
  newTcpClient.sock = TCPClient_sock;
  newTcpClient.mTCPClientObject = jerry_acquire_value(TCPClient_object_value); // TODO: release!
  printf("newTcpClient.mTCPClientObject = %d\n", newTcpClient.mTCPClientObject);



  bool found = false;
  pthread_mutex_lock(mutex1);
  for (TCPServerListIterator i1 = tcpServerList.begin(); i1 != tcpServerList.end(); ++i1)
  {
    TCPServer &tcpServer = *i1;

    if (tcpServer.mSock == TCPServer_sock)
    {
      SocketList &clientList = tcpServer.mClientList;
      if (std::find(clientList.begin(), clientList.end(), newTcpClient) == clientList.end()) // only add client to client list if it is not already there
      {
        clientList.push_back(newTcpClient);
        puts("added TCPClient to TCPServer");
      }
      else
        puts("TCPClient already in TCPServer");

      found = true;
      break;
    }
  }
  pthread_mutex_unlock(mutex1);

  if (!found)
  {
    puts("oiiiiidjidjdkdkdkdk");
    return jerry_create_boolean(false);
  }
  else
  {
    struct epoll_event event;

    event.data.fd = TCPClient_sock;
    event.events = EPOLLIN | EPOLLET;

    if (epoll_ctl(efd, EPOLL_CTL_ADD, TCPClient_sock, &event) < 0)
    {
      puts("!epoll_ctl could not add new client to epoll_ctl");
      return jerry_create_boolean(false);
    }
    else
      puts("added new client to epoll_ctl");
  }



  puts("JO!!");


  return jerry_create_boolean(true);
}





// NOTE: does not match with original connect(..) function, as this would not work that easily, and why make it difficult
static jerry_value_t internal_connect_handler(const jerry_value_t function_obj,
                                              const jerry_value_t this_val,
                                              const jerry_value_t args[],
                                              const jerry_length_t argc)
{
  //puts("internal_connect_handler called!");

  if (argc != 3)
  {
    puts("posdpokfd");
    return jerry_create_boolean(false);
  }

  jerry_value_t TCPClient_object_value = args[0];

  if (!jerry_value_is_object(TCPClient_object_value))
  {
    puts("ijoijdoijfdoijfdoij");
    return jerry_create_boolean(false);
  }


  jerry_value_t host_value = args[1];

  char *host = value_to_string(host_value);

  if (host == nullptr)
  {
    puts("iojsoijoijj,m,,,");
    return jerry_create_boolean(false);
  }


  jerry_value_t port_value = args[2];

  if (!jerry_value_to_number(port_value))
  {
    puts("ööösäsäsöls");
    return jerry_create_boolean(false);
  }

  uint16_t port = jerry_get_number_value(port_value);

  //printf("port = %d\n", port);





  jerry_value_t prop_name = jerry_create_string((const jerry_char_t *)"socket");
  jerry_value_t socket_object_value = jerry_get_property(TCPClient_object_value, prop_name);




  handle_container_t *handle_container = nullptr;
  bool has_file_handle = jerry_get_object_native_pointer(socket_object_value, (void**)&handle_container, 0);

  if (!has_file_handle || handle_container == nullptr)
  {
    puts("no valid socket handle");
    return jerry_create_boolean(false);
  }

  int sock = handle_container->handle;

  //printf("sock = %d\n", sock);







  struct sockaddr_in server;
  uint32_t ipv4 = 0;

  memset(&server, 0, sizeof(server));

  ipv4 = inet_addr(host);

  if (ipv4 != INADDR_NONE) // is ipv4
  {
    memcpy((char *)&server.sin_addr, &ipv4, sizeof(ipv4));
    //printf("ipv4! %d\n", server.sin_addr);
  }
  else // is probably a hostname
  {
    struct hostent *hostInfo;
    hostInfo = gethostbyname(host);
    if (hostInfo == nullptr)
    {
      printf("host name cannot be resolved: %s - errno: %d\n", host, h_errno);
      return jerry_create_boolean(false);
    }

    memcpy((char *)&server.sin_addr, hostInfo->h_addr, hostInfo->h_length);
    //printf("hostname! %d\n", server.sin_addr);
  }

  server.sin_family = AF_INET;
  server.sin_port = htons(port);


  int result = connect(sock, (struct sockaddr*)&server, sizeof(server));

  if (result < 0 && errno != EINPROGRESS /* for non-blocking sockets which don't connect immediately */)
  {
    puts("!connect");

    printf("errno = %d\n", errno);

    return jerry_create_boolean(false);
  }




  {
    struct epoll_event event;

    event.data.fd = sock;
    event.events = EPOLLOUT | EPOLLIN | EPOLLERR | EPOLLET;

    if (epoll_ctl(efd, EPOLL_CTL_ADD, sock, &event) < 0)
    {
      puts("!epoll_ctl");
      return jerry_create_boolean(false);
    }
  }


  pthread_mutex_lock(mutex1);
  TCPClient tcpClient;
  tcpClient.sock = sock;
  tcpClient.mTCPClientObject = TCPClient_object_value;
  pendingConnects.push_back(tcpClient);
  pthread_mutex_unlock(mutex1);


  //puts("put new connection on pendingConnects");


  return jerry_create_boolean(true);
}





static jerry_value_t internal_write_handler(const jerry_value_t function_obj,
                                            const jerry_value_t this_val,
                                            const jerry_value_t args[],
                                            const jerry_length_t argc)
{
  //puts("internal_write_handler called");

  if (argc != 3)
  {
    puts("uieduiojefj");
    return jerry_create_number(0);
  }


  jerry_value_t socket_value = args[0];

  if (!jerry_value_is_object(socket_value))
  {
    puts("uiedusdsdsdsdsdsdiojefj");
    return jerry_create_number(0);
  }



  handle_container_t *handle_container = nullptr;
  bool has_file_handle = jerry_get_object_native_pointer(socket_value, (void**)&handle_container, 0);

  if (!has_file_handle || handle_container == nullptr)
  {
    puts("no valid socket handle");
    return jerry_create_number(0);
  }

  int sock = handle_container->handle;

  //printf("sock = %d\n", sock);



  jerry_value_t buffer_value = args[1];


  if (!jerry_value_is_arraybuffer(buffer_value))
  //if (!jerry_value_is_typedarray(buffer_value))
  {
    puts("ijodoidiodididididididkm,,,,");
    return jerry_create_number(0);
  }


  jerry_value_t buffer_len_value = args[2];

  if (!jerry_value_is_number(buffer_len_value))
  {
    puts("999922222222mdjnfdinf");
    return jerry_create_number(0);
  }


  uint64_t buffer_len = jerry_get_number_value(buffer_len_value);
  //printf("buf len = %" PRIu64 "\n", buffer_len);


  uint8_t *buf = (uint8_t *)malloc(buffer_len);
  memset(buf, 1, buffer_len);


  jerry_arraybuffer_read(buffer_value, 0, buf, buffer_len);


  // printf("buf = ");

  // for (int i = 0; i < buffer_len; i++)
  // {
  //   printf("%d, ", buf[i]);
  // }


  jerry_value_t result = jerry_create_number(write(sock, buf, buffer_len));

  free(buf);

  return result;
}







static jerry_value_t internal_close_handler(const jerry_value_t function_obj,
                                             const jerry_value_t this_val,
                                             const jerry_value_t args[],
                                             const jerry_length_t argc)
{
  //puts("internal_close_handler called");


  if (argc != 1)
  {
    puts("uieduiojefjssssssss");
    return jerry_create_number(0);
  }



  jerry_value_t TCPClient_object_value = args[0];

  if (!jerry_value_is_object(TCPClient_object_value))
  {
    puts("jnjnnnnnnnnnnnssdsddsds");
    return jerry_create_number(0);
  }



  jerry_value_t prop_name = jerry_create_string((const jerry_char_t *)"socket");
  jerry_value_t socket_object_value = jerry_get_property(TCPClient_object_value, prop_name);

  if (!jerry_value_is_object(socket_object_value))
  {
    puts("jnjnnnnnnnnnnn");
    return jerry_create_number(0);
  }



  handle_container_t *handle_container = nullptr;
  bool has_file_handle = jerry_get_object_native_pointer(socket_object_value, (void**)&handle_container, 0);

  if (!has_file_handle || handle_container == nullptr)
  {
    puts("no valid socket handle");
    return jerry_create_boolean(false);
  }

  int sock = handle_container->handle;

  //printf("sock = %d\n", sock);



  pthread_mutex_lock(mutex1);
  workerEventList.push_back(new DiscoEvent(sock));
  pthread_mutex_unlock(mutex1);



  return jerry_create_number(close(sock));
}



static jerry_value_t internal_accept_handler(const jerry_value_t function_obj,
                                             const jerry_value_t this_val,
                                             const jerry_value_t args[],
                                             const jerry_length_t argc)
{
  //puts("internal_accept_handler called");

  if (argc != 1)
  {
    puts("uieduiojefj");
    return jerry_create_number(0);
  }


  jerry_value_t socket_value = args[0]; // NOTE: this is the SERVER socket

  if (!jerry_value_is_object(socket_value))
  {
    puts("uiedusdsdsdsdsdsdiojefj");
    return jerry_create_number(0);
  }



  handle_container_t *handle_container = nullptr;
  bool has_file_handle = jerry_get_object_native_pointer(socket_value, (void**)&handle_container, 0);

  if (!has_file_handle || handle_container == nullptr)
  {
    puts("no valid socket handle");
    return jerry_create_number(0);
  }

  int sock = handle_container->handle;



  int newClient = accept(sock, 0, 0);

  if (newClient < 0)
  {
    //puts("internal_accept_handler: no new connection");
    return jerry_create_number(0);
  }





  jerry_value_t handle_object = jerry_create_object();
  static const jerry_object_native_info_t native_obj_type_info =
  {
    .free_cb = native_close_cb
  };


  handle_container_t *handle_container2 = (handle_container_t *)malloc(sizeof(handle_container_t));
  handle_container2->handle = newClient;

  jerry_set_object_native_pointer(handle_object, handle_container2, &native_obj_type_info);



  return handle_object;
}










struct Reads
{
  int clientSocket;
  uint8_t *buffer;
  uint32_t readSize;
};

std::list<Reads> readsList;








bool run = true;



void worker_event_loop()
{
  #define ReadBufferSize 10 * 1024 * 1024 // 10MB
  uint8_t *readBuffer = (uint8_t *)malloc(ReadBufferSize + 1);

  while (run)
  {
    pthread_mutex_lock(mutex1);
    if (readyReadSockets.empty() && workerEventList.empty())
    {
      pthread_mutex_unlock(mutex1);
      usleep(10 * 1000);
      continue;
    }

        pthread_mutex_lock(mutex1);
    if (!workerEventList.empty())
    {
      AbstractWorkerEvent *event = workerEventList.front();
      workerEventList.pop_front();

      switch (event->mType)
      {
        case AbstractWorkerEvent::Type::DiscoEvent:
        {
          puts("DiscoEvent!");
          DiscoEvent *devent = dynamic_cast<DiscoEvent *>(event);

          SocketListIterator findClient = std::find(freeStandingTCPClients.begin(), freeStandingTCPClients.end(), devent->mSock);

          if (findClient != freeStandingTCPClients.end()) // is the client a freestanding client?
          {
            close(devent->mSock);
            freeStandingTCPClients.erase(findClient);
            //puts("was freestanding");
          }
          else  // or is it a connection referencing to a tcp server?
          {
            // look in all TCPServers for clients !!! NOTE: client can only belong to one server
            bool found = false;
            for (TCPServerListIterator i1 = tcpServerList.begin(); i1 != tcpServerList.end(); ++i1)
            {
              TCPServer &tcpServer = *i1;
              SocketList &clientList = tcpServer.mClientList;

              for (SocketListIterator i2 = clientList.begin(); i2 != clientList.end(); ++i2)
              {
                TCPClient &tcpClient = *i2;
                if (tcpClient.sock == devent->mSock)
                {
                  clientList.erase(i2); // finally remove client from server
                  found = true;
                  //puts("found!");
                  break;
                }
              }

              if (found)
                break;
            }

            if (!found) // still not found? hmmm... TODO: are there cases?
              puts("WARNING: cannot remove client as it was not found!");
          }

          break;
        }
      }
    }


    if (!readyReadSockets.empty())
    {
      if (readsList.size() >= 1000 /* memory has its limits */)
      {
        puts("readsList full! waiting until it reduces...");
        pthread_mutex_unlock(mutex1);
        continue;
      }

      //puts("a socket is ready read!");

      int clientSocket = readyReadSockets.front();
      readyReadSockets.pop_front();

      pthread_mutex_unlock(mutex1);

      memset(readBuffer, ReadBufferSize + 1 /* sec */, 1); // sec
      ssize_t size = read(clientSocket, readBuffer, ReadBufferSize);

      if (size < 1) // client disconnected
      {
        puts("client disco!");
        close(clientSocket);

        // also remove client from client list of server
        pthread_mutex_lock(mutex1);
        for (TCPServerListIterator i1 = tcpServerList.begin(); i1 != tcpServerList.end(); ++i1)
        {
          TCPServer &tcpServer = *i1;
          SocketList &clientList = tcpServer.mClientList;

          bool removed = false;
          for (SocketListIterator i2 = clientList.begin(); i2 != clientList.end(); ++i2)
          {
            TCPClient &tcpClient = *i2;

            if (tcpClient.sock == clientSocket)
            {
              tcpServerList.erase(i1);
              puts("removed client from server's client list");
              removed = true;
              break;
            }
          }

          if (removed)
            break;
        }
        pthread_mutex_unlock(mutex1);

        continue;
      }

      if (size == ReadBufferSize)
      {
        puts("WARNING: worker_event_loop: socket may have reached the 10M receiving buffer limit!");
      }

      // TODO: close client connection if there is more data? otherwise the connection gets stuck. close because someone tries to spam?

      //printf("> %s\n", readBuffer);
      pthread_mutex_lock(mutex1);
      readsList.push_back( { clientSocket, readBuffer, (uint32_t /* TODO: ! */)size } );
      pthread_mutex_unlock(mutex1);
    }
  }

  free(readBuffer);
}








void script_event_loop()
{
  // feed receive
  pthread_mutex_lock(mutex1);

  if (!readsList.empty())
  {
    struct Reads reads = readsList.front();
    readsList.pop_front();

    TCPServerList tcpServerListCopy = tcpServerList;

    pthread_mutex_unlock(mutex1);

    bool processed = false;
    for (TCPServerListIterator i1 = tcpServerListCopy.begin(); i1 != tcpServerListCopy.end(); ++i1)
    {
      TCPServer &tcpServer = *i1;

      SocketList &clientSocketList = tcpServer.mClientList;

      SocketListIterator findClientSocket = std::find(clientSocketList.begin(), clientSocketList.end(), reads.clientSocket);


      if (findClientSocket != clientSocketList.end())
      {
        TCPClient &tcpClient = *findClientSocket;

        //jerry_value_t prototype = jerry_get_prototype(tcpClient.mTCPClientObject);

        jerry_value_t prop_name = jerry_create_string((const jerry_char_t *)"receive");
        //printf("tcpClient.mTCPClientObject = %d\n", tcpClient.mTCPClientObject);
        jerry_value_t receive_function = jerry_get_property(tcpClient.mTCPClientObject, prop_name);


        if (!jerry_value_is_function(receive_function))
        {
          puts("not a function :(");
          return;
        }

        // int argc = 1;
        // jerry_value_t args[argc];

        // args[0] = jerry_create_number(777);

        //printf("feeding from %d with size %" PRIu32 "\n", reads.clientSocket, reads.readSize);

        jerry_value_t buffer = jerry_create_arraybuffer(reads.readSize);

        jerry_arraybuffer_write(buffer, 0, reads.buffer, reads.readSize);

        // TODO: free buffer of reads now

        int argc = 1;
        jerry_value_t args[argc];

        args[0] = buffer;

        processed = true;

        jerry_call_function(receive_function, tcpClient.mTCPClientObject, args, argc);
        return; // parent tcp server found, so don't look any further
      }
    }


    // there are connections that are not from a client of a server but a self made connection, they are listed in the free standing list... i don't like that solution ...
    if (!processed)
    {
      TCPClient test; // needed to find in list... grrr
      test.sock = reads.clientSocket;

      pthread_mutex_lock(mutex1);
      SocketListIterator findTCPClient = std::find(freeStandingTCPClients.begin(), freeStandingTCPClients.end(), test);

      if (findTCPClient != freeStandingTCPClients.end())
      {
        TCPClient &tcpClient = *findTCPClient;

        jerry_value_t TCPClient_object = tcpClient.mTCPClientObject;
        pthread_mutex_unlock(mutex1);

        /// CODE DUPLICATE ///


        jerry_value_t prop_name = jerry_create_string((const jerry_char_t *)"receive");
        //printf("TCPClient_object = %d\n", TCPClient_object);
        jerry_value_t receive_function = jerry_get_property(TCPClient_object, prop_name);


        if (!jerry_value_is_function(receive_function))
        {
          puts("not a function :(");
          return;
        }

        // int argc = 1;
        // jerry_value_t args[argc];

        // args[0] = jerry_create_number(777);

        //printf("feeding from %d with size %" PRIu32 "\n", reads.clientSocket, reads.readSize);

        jerry_value_t buffer = jerry_create_arraybuffer(reads.readSize);

        jerry_arraybuffer_write(buffer, 0, reads.buffer, reads.readSize);

        int argc = 1;
        jerry_value_t args[argc];

        args[0] = buffer;

        processed = true;

        jerry_call_function(receive_function, TCPClient_object, args, argc);
        return; // parent tcp server found, so don't look any further
      }
      else
        pthread_mutex_unlock(mutex1);


      puts("not found.......!");
    }


    if (!processed)
    {
      puts("damn...");
    }

  }
  else
    pthread_mutex_unlock(mutex1);



  // feed new clients



  // check Events for Script
  pthread_mutex_lock(mutex1);
  if (!scriptEventList.empty())
  {
    puts("new script event !");
    AbstractScriptEvent *event = scriptEventList.front(); // we take the event, so everyting inside is now belongs to us, no need to lock anymore!
    scriptEventList.pop_front();
    pthread_mutex_unlock(mutex1);

    switch (event->mType)
    {
      case AbstractScriptEvent::Type::FailedConnectEvent:
      {
        puts("FailedConnectEvent");
        FailedConnectEvent *fcevent = dynamic_cast<FailedConnectEvent *>(event);

        if (!fcevent)
          puts("!fcevent");

        TCPClient &tcpClient = fcevent->mTcpCLient; // TODO: without lock??! but GC cannot run before calling something in jerryscript

        jerry_value_t prop_name = jerry_create_string((const jerry_char_t *)"error");
        jerry_value_t error_function = jerry_get_property(tcpClient.mTCPClientObject, prop_name);

        jerry_value_t result = jerry_call_function(error_function, tcpClient.mTCPClientObject, 0, 0);

        if (jerry_value_is_error(result))
        {
          //jerry_value_clear_error_flag(&result);
          printf("err: %d\n", jerry_get_error_type(result));
        }

        return;
      }

      case AbstractScriptEvent::Type::ConnectedEvent:
      {
        puts("ConnectedEvent");

        ConnectedEvent *cevent = dynamic_cast<ConnectedEvent *>(event);

        if (!cevent)
          puts("!cevent");

        TCPClient &tcpClient = cevent->mTcpCLient;

        jerry_value_t prop_name = jerry_create_string((const jerry_char_t *)"connected");
        jerry_value_t connected_function = jerry_get_property(tcpClient.mTCPClientObject, prop_name);

        jerry_call_function(connected_function, tcpClient.mTCPClientObject, 0, 0);

        return;
      }
    }


  }
  else
    pthread_mutex_unlock(mutex1);



  pthread_mutex_lock(mutex1);
  for (TCPServerListIterator i1 = tcpServerList.begin(); i1 != tcpServerList.end(); ++i1)
  {
    TCPServer &tcpServer = *i1;

    if (tcpServer.mHasPendingConnections)
    {
      tcpServer.mHasPendingConnections = false; // new connection must be handled in JS
      puts("yep, new connections!");
      //jerry_value_t prototype = jerry_get_prototype(tcpServer.mTCPServerObject);

      jerry_value_t prop_name = jerry_create_string((const jerry_char_t *)"newConnection");
      jerry_value_t receive_function = jerry_get_property(tcpServer.mTCPServerObject, prop_name);


      if (!jerry_value_is_function(receive_function))
      {
        puts("not a function :(");
        return;
      }

      pthread_mutex_unlock(mutex1);
      // never execute the vm while a mutex is locked !
      jerry_call_function(receive_function, tcpServer.mTCPServerObject, 0, 0);
      pthread_mutex_lock(mutex1);
    }
  }

  pthread_mutex_unlock(mutex1);

  // end feed to javascript

  jerry_release_value(jerry_run_all_enqueued_jobs());
}


void *networking(void *arg)
{
  (void)arg;


  #define MaxEvents 100
  struct epoll_event *eventList;
  eventList = (struct epoll_event *)calloc(MaxEvents, sizeof(struct epoll_event));


  while (run) // event loop :)
  {
    usleep(10*1000);
    int pending = epoll_wait(efd, eventList, MaxEvents, 1000); // TODO: timeout

    for (int i1 = 0; i1 < pending; i1++)
    {
      struct epoll_event event = eventList[i1];
      uint32_t events = event.events;

      if (events & EPOLLERR || events & EPOLLHUP) //|| !(events & EPOLLIN))
      {
        // if (event.data.fd == svr)
        // {
        //   exit(-99);
        // }

        // probably a connect(..) call that failed (invalid hostname, host offline, etc.)
        pthread_mutex_lock(mutex1);
        SocketListIterator findClient = std::find(pendingConnects.begin(), pendingConnects.end(), event.data.fd);

        bool found = false;

        if (findClient != pendingConnects.end()) // found
        {
          TCPClient &tcpClient = *findClient;

          scriptEventList.push_back(new FailedConnectEvent(tcpClient));

          puts("found!");
          found = true;
        }
        else
        {
          SocketListIterator findClient = std::find(freeStandingTCPClients.begin(), freeStandingTCPClients.end(), event.data.fd);

          if (findClient != freeStandingTCPClients.end()) // is the client a freestanding client?
          {
            close(event.data.fd);
            freeStandingTCPClients.erase(findClient);
            found = true;
            //puts("was freestanding");
          }
          else
          {
            // look in all TCPServers for clients !!! NOTE: client can only belong to one server
            for (TCPServerListIterator i1 = tcpServerList.begin(); i1 != tcpServerList.end(); ++i1)
            {
              TCPServer &tcpServer = *i1;
              SocketList &clientList = tcpServer.mClientList;

              for (SocketListIterator i2 = clientList.begin(); i2 != clientList.end(); ++i2)
              {
                TCPClient &tcpClient = *i2;
                if (tcpClient.sock == event.data.fd)
                {
                  clientList.erase(i2); // finally remove client from server
                  found = true;
                  //puts("found!");
                  break;
                }
              }

              if (found)
                break;
            }
          }
        }
        pthread_mutex_unlock(mutex1);

        if (!found) // still not found? hmmm... TODO: are there cases?
          puts("WARNING: cannot remove client as it was not found!");
        else
          puts("found & closed!");


        puts("connect error or host closed connection");
        close(event.data.fd);
        continue;
      }



      // look if any server socket has pending connections

      pthread_mutex_lock(mutex1);
      //printf("tcpServerList size = %d\n", tcpServerList.size());
      TCPServerList tcpServerListCopy = tcpServerList;
      pthread_mutex_unlock(mutex1);


      bool wasAServerSocketEvent = false;

      for (TCPServerListIterator i2 = tcpServerListCopy.begin(); i2 != tcpServerListCopy.end(); ++i2)
      {
        TCPServer &tcpServer = *i2;

        //printf("checking %d\n", tcpServer.mSock);

        if (event.data.fd == tcpServer.mSock)
        {
          wasAServerSocketEvent = true;

          //while (1) // accept all pending connections
          {
            // add notice to TCPServer that there is a (are) new incoming connection(s)
            pthread_mutex_lock(mutex1);
            {
              TCPServerListIterator findTCPServer = std::find(tcpServerList.begin(), tcpServerList.end(), tcpServer); // get the live version of this tcpServer

              if (findTCPServer != tcpServerList.end())
              {
                TCPServer &liveTcpServer = *findTCPServer;

                liveTcpServer.mHasPendingConnections = true; // TODO: to event !
                puts("set new pending connections to true");
              }
              else
                puts("WTFUQUE !!");
            }
            pthread_mutex_unlock(mutex1);
          }
        }
      }

      if (!wasAServerSocketEvent) // was an event on client socket(s) ?
      {
        if (events & EPOLLOUT) // a socket which was connected via connect(..) got ready
        {
          bool found = false;
          pthread_mutex_lock(mutex1);
          for (SocketListIterator i1 = pendingConnects.begin(); i1 != pendingConnects.end(); ++i1)
          {
            TCPClient &tcpClient = *i1;

            if (tcpClient.sock == event.data.fd)
            {
              pendingConnects.erase(i1); // remove this pending connection from the list as the connection has been established now
              found = true;
              //puts("found!!");


              // look for ready read on the new opened socket
              {
                struct epoll_event event2;

                event2.data.fd = event.data.fd;
                event2.events = EPOLLIN | EPOLLET;

                if (epoll_ctl(efd, EPOLL_CTL_MOD /* as the current socket is already added, but with EPOLLOUT, so MOD it */, event.data.fd, &event2) < 0)
                {
                  puts("!epoll_ctl");
                }
              }


              freeStandingTCPClients.push_back(tcpClient);

              scriptEventList.push_back(new ConnectedEvent(tcpClient));

              break;
            }
          }
          pthread_mutex_unlock(mutex1);

          if (!found)
            puts("not found ....");

        }
        else
        {
          readyReadSockets.push_back(event.data.fd);
          //puts("added to readyReadSockets");
        }
      }
    }
  }


  puts("networking thread end");
  return nullptr;
}


pthread_t *networking_thread;


//int efd = 0;


void wrapper_init()
{
  puts("init net wrapper");

  mutex1 = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(mutex1, 0);

  efd = epoll_create1(0);

  networking_thread = (pthread_t *)malloc(sizeof(pthread_t));
  pthread_create(networking_thread, 0, networking, 0);



  jerry_value_t glob_obj = jerry_get_global_object();

  jerry_value_t internal_obj = jerry_create_object();
  jerry_value_t internal_obj_name = jerry_create_string((const jerry_char_t *)"Internal");
  jerry_release_value(jerry_set_property(glob_obj, internal_obj_name, internal_obj));
  jerry_release_value(internal_obj_name);

  jerry_release_value(glob_obj);


  jsbx_add_js_function("socket", internal_socket_handler, internal_obj);
  jsbx_add_js_function("accept", internal_accept_handler, internal_obj);
  jsbx_add_js_function("c_bind", internal_bind_handler, internal_obj);
  jsbx_add_js_function("listen", internal_listen_handler, internal_obj);
  jsbx_add_js_function("registerTCPServer", internal_registerTCPServer_handler, internal_obj);
  jsbx_add_js_function("registerTCPClient", internal_registerTCPClient_handler, internal_obj);
  jsbx_add_js_function("write", internal_write_handler, internal_obj);
  jsbx_add_js_function("connect", internal_connect_handler, internal_obj);
  jsbx_add_js_function("close", internal_close_handler, internal_obj);


  jerry_release_value(internal_obj);

  jsbx_include("../net/net.js");

  jsbx_register_worker_event_handler(&worker_event_loop);
  jsbx_register_script_event_handler(&script_event_loop);
}


void wrapper_free()
{
  run = false;
  pthread_join(*networking_thread, 0);
  pthread_mutex_destroy(mutex1);
}
