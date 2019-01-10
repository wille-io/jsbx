function test()
{
  print("test.js");

  load("/home/wille/projects/jsbx/build/net/libNet.so");
  print("1");
  
  var z = new TCPClient();
  print("2");
    
  z.connect("192.168.133.200", 80);
  print("3");
}

test();