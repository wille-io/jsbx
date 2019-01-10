
function test()
{
  print("wrapper-test: test.js");
  load("/home/wille/projects/jsbx/build/botan-wrapper/libBotanWrapper.so");
  print("here1");
  test123_JS();
  print("here3");
}

test();