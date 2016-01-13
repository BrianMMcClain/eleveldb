#include "Encoding.h"

#include "exceptionutils.h"

using namespace std;

using namespace eleveldb;

/**.......................................................................
 * Constructor.
 */
Encoding::Encoding() {}

/**.......................................................................
 * Destructor.
 */
Encoding::~Encoding() {}

ostream& 
eleveldb::operator<<(ostream& os, Encoding::Type type)
{
  switch (type) {
  case Encoding::EI:
      os << "EI";
      break;
  case Encoding::PB:
      os << "PB";
      break;
  case Encoding::ERLANG:
      os << "ERLANG";
      break;
  case Encoding::MSGPACK:
      os << "MSGPACK";
      break;
  default:
      os << "UNKNOWN";
      break;
  }

  return os;
}

std::string Encoding::encodingAtom(Encoding::Type type)
{
    switch(type) {
    case Encoding::PB:
        return "pb";
        break;
    case Encoding::EI:
        return "ei";
        break;
    case Encoding::ERLANG:
        return "erlang";
        break;
    case Encoding::MSGPACK:
        return "msgpack";
        break;
    default:
        return "unknown";
        break;
    }
}

Encoding::Type Encoding::typeOf(std::string str, bool doThrow)
{
    if(str == encodingAtom(PB))
        return PB;

    if(str == encodingAtom(EI))
        return EI;

    if(str == encodingAtom(ERLANG))
        return ERLANG;

    if(str == encodingAtom(MSGPACK))
        return MSGPACK;

    if(doThrow)
        ThrowRuntimeError("Unrecognized encoding type: " << str);

    return UNKNOWN;
}

