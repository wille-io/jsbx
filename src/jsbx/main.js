'use strict';

// temporarily including File class here:

SeekEnum =
{
  SEEK_SET: 0,
  SEEK_CUR: 1,
  SEEK_END: 2
};




function File(filename, mode)
{
  this.handle = Internal.fopen(filename, mode);
  this.position = 0;
  //print("handle = " + this.handle);
}


File.prototype.isOpen = function()
{
  return (this.handle === 0) ? false : true;
}


File.prototype.seek = function(position)
{
  print("File.seek");

  if (position == this.position) // nothing to do if already at that position
  {
    print("no need to seek");
    return 0;
  }

  return Internal.fseek(this.handle, position, SEEK_SET);
};


File.prototype.read = function(buf, position, bytes)
{
  print("File.read");

  if (this.seek(position) != 0)
  {
    print("cannot seek!");
    return 0;
  }

  var result = Internal.fread(buf, 1, bytes, this.handle);

  this.position += result; // add read bytes to result as fread moves the file cursor

  return result;
};




// function main()
// {
//   print("wrong main!!");
// }




function hello()
{
  print("jsbx v0 - ready");
}

hello();