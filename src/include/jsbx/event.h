#pragma once

#include <list>


class AbstractScriptEvent
{
public:
  enum class Type
  {
    FailedConnectEvent,
    ConnectedEvent,
  };

  AbstractScriptEvent(Type type)
    : mType(type)
  {}

  virtual ~AbstractScriptEvent() = default;

  Type mType;
};


typedef std::list<AbstractScriptEvent *> ScriptEventList;
typedef ScriptEventList::iterator ScriptEventListIterator;


class AbstractWorkerEvent
{
public:
  enum class Type
  {
    DiscoEvent,
  };

  AbstractWorkerEvent(Type type)
    : mType(type)
  {}

  virtual ~AbstractWorkerEvent() = default;

  Type mType;
};


typedef std::list<AbstractWorkerEvent *> WorkerEventList;
typedef WorkerEventList::iterator WorkerEventListIterator;


std::list<AbstractScriptEvent *> scriptEventList; // Events from Threads to Script
std::list<AbstractWorkerEvent *> workerEventList; // Events from Script to Threads
