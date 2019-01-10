AFEnum = 
{
  AF_INET: 2
};


SOCKEnum = 
{
  SOCK_STREAM: 1
};


function TCPServer()
{

}


TCPServer.prototype.startServer = function(port)
{
  this.port = port;
  this.socket = Internal.socket(2, 1, 0);

  print("socket = " + this.socket);

  if (this.socket !== 0)
  {
    if (Internal.c_bind(this.socket, 9999))
    {
      if (Internal.listen(this.socket, 20))
      {
        if (Internal.registerTCPServer(this, this.socket))
        {
          this._errorCode = 0;
          return;
        }
        else
          print("!registerTCPServerfsdfsdfsd");
      }
      else
        print("!listen");
    }
    else
      print("!bind");
  }
  else
    print("!socket");

  this._errorCode = Internal.errno();
}


TCPServer.prototype.acceptConnection = function()
{
  var newClientSocket = Internal.accept(this.socket);
  
  if (newClientSocket < 1)
    return 0;

  var tcpClient = new TCPClient(newClientSocket);

  Internal.registerTCPClient(this.socket /* TCPServer */, tcpClient);

  return tcpClient;
};



function TCPClient(socket)
{
  this.socket = socket;
  this._errorCode = 0;
}


TCPClient.prototype.errorString = function()
{
  return Internal.errorString(this._errorCode);
}


TCPClient.prototype.sendText = function(text)
{
  var buf = new ArrayBuffer(text.length*2);
  var arr = new Uint16Array(buf);

  for (var i1 = 0; i1 < text.length; i1++)
  {
    //print("char = " + text.charCodeAt(i1));
    arr[i1] = text.charCodeAt(i1);
  }

  print("len = " + arr.buffer.byteLength);
  print("write returned " + Internal.write(this.socket, buf, buf.byteLength));
};


TCPClient.prototype.connect = function(host, port)
{
  this.socket = Internal.socket(2, 1, 0);
  return Internal.connect(this, host, port);
}


TCPClient.prototype.disconnect = function()
{
  Internal.close(this);
}