namespace leveldb
{
  export class Options
  {
    constructor() {}
    test: string;
  }

  export declare function Open(options: Options, dbPath: string, dbObject: any): any;
}