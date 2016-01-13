#include "ErlUtil.h"

#include "cmp.h"
#include "cmp_mem_access.h"

#include <cmath>

#include <arpa/inet.h>
#include <xlocale.h>

using namespace std;

using namespace eleveldb;

/**.......................................................................
 * Constructors
 */
ErlUtil::ErlUtil(ErlNifEnv* env) 
{
    setEnv(env);
    hasTerm_ = 0;
}

ErlUtil::ErlUtil(ErlNifEnv* env, ERL_NIF_TERM term) 
{
    setEnv(env);
    setTerm(term);
}

/**.......................................................................
 * Destructor.
 */
ErlUtil::~ErlUtil() {}

void ErlUtil::setEnv(ErlNifEnv* env) 
{
    env_ = env;
}

void ErlUtil::setTerm(ERL_NIF_TERM term) 
{
    term_ = term;
    hasTerm_ = true;
}

void ErlUtil::checkEnv() 
{
    if(!env_)
        ThrowRuntimeError("No environment has been set");
}

void ErlUtil::checkTerm() 
{
    if(!term_)
        ThrowRuntimeError("No term has been set");
}

bool ErlUtil::isAtom()
{
    checkTerm();
    return isAtom(term_);
}

bool ErlUtil::isAtom(ERL_NIF_TERM term)
{
    checkEnv();
    return isAtom(env_, term);
}

bool ErlUtil::isAtom(ErlNifEnv* env, ERL_NIF_TERM term)
{
    return enif_is_atom(env, term);
}

bool ErlUtil::isList()
{
    checkTerm();
    return isList(term_);
}

bool ErlUtil::isList(ERL_NIF_TERM term)
{
    checkEnv();
    return isList(env_, term);
}

bool ErlUtil::isList(ErlNifEnv* env, ERL_NIF_TERM term)
{
    return enif_is_list(env, term);
}

bool ErlUtil::isBinary()
{
    checkTerm();
    return isBinary(term_);
}

bool ErlUtil::isBinary(ERL_NIF_TERM term)
{
    checkEnv();
    return isBinary(env_, term);
}

bool ErlUtil::isBinary(ErlNifEnv* env, ERL_NIF_TERM term)
{
    return enif_is_binary(env, term);
}

bool ErlUtil::isInspectableAsBinary()
{
    checkTerm();
    return isInspectableAsBinary(term_);
}

bool ErlUtil::isInspectableAsBinary(ERL_NIF_TERM term)
{
    checkEnv();
    return isInspectableAsBinary(env_, term_);
}

bool ErlUtil::isInspectableAsBinary(ErlNifEnv* env, ERL_NIF_TERM term)
{
    try {
        getAsBinary(env, term);
        return true;
    } catch(...) {
        return false;
    }
}

bool ErlUtil::isTuple()
{
    checkTerm();
    return isTuple(term_);
}

bool ErlUtil::isTuple(ERL_NIF_TERM term)
{
    checkEnv();
    return isTuple(env_, term);
}

bool ErlUtil::isTuple(ErlNifEnv* env, ERL_NIF_TERM term)
{
    return enif_is_tuple(env, term);
}

bool ErlUtil::isString()
{
    checkTerm();
    return isString(term_);
}

bool ErlUtil::isString(ERL_NIF_TERM term)
{
    checkEnv();
    return isString(env_, term);
}

bool ErlUtil::isString(ErlNifEnv* env, ERL_NIF_TERM term)
{
    unsigned size=0;

    if(isAtom(env, term)) {
        return true;
        
        //------------------------------------------------------------
        // If this is a binary, it _could_ be a string
        //------------------------------------------------------------
        
    } else if(isBinary(env, term)) {
        ErlNifBinary bin;

        if(enif_inspect_binary(env, term, &bin) == 0) {
            ThrowRuntimeError("Failed to inspect '" << formatTerm(env, term)
                              << "' as a binary");
        }

        size = bin.size;
        for(unsigned i=0; i < size; i++) {
            if(!isprint_l(bin.data[i], LC_GLOBAL_LOCALE))
                return false;
        }

        return true;

        //------------------------------------------------------------
        // Erlang represents strings internally as lists, so use native
        // erlang conversion to check a list
        //------------------------------------------------------------

    } else if(isList(env, term)) {

        if(enif_get_list_length(env, term, &size) == 0) {
            ThrowRuntimeError("Failed to get list length");
        }

        char buf[size+1];

        // Note that enif_get_string expects the buffer size, not the
        // string length, and will return less than the length of the
        // string if the passed size isn't large enough to include the
        // string + null terminator

        return enif_get_string(env, term, buf, size+1, ERL_NIF_LATIN1);

        //------------------------------------------------------------
        // Else not a string
        //------------------------------------------------------------

    } else {
        return false;
    }
}

bool ErlUtil::isNumber()
{
    checkTerm();
    return isNumber(term_);
}

bool ErlUtil::isNumber(ERL_NIF_TERM term)
{
    checkEnv();
    return isNumber(env_, term);
}

bool ErlUtil::isNumber(ErlNifEnv* env, ERL_NIF_TERM term)
{
    return enif_is_number(env, term);
}

bool ErlUtil::isBool()
{
    checkTerm();
    return isBool(term_);
}

bool ErlUtil::isBool(ERL_NIF_TERM term)
{
    checkEnv();
    return isBool(env_, term);
}

bool ErlUtil::isBool(ErlNifEnv* env, ERL_NIF_TERM term)
{
    if(isAtom(env, term)) {
        std::string atom = getAtom(env, term, true);
        if(atom == "true") {
            return true;
        } else if(atom == "false") {
            return true;
        }
    }

    return false;
}

bool ErlUtil::isDouble()
{
    checkTerm();
    return isDouble(term_);
}

bool ErlUtil::isDouble(ERL_NIF_TERM term)
{
    checkEnv();
    return isDouble(env_, term);
}

bool ErlUtil::isDouble(ErlNifEnv* env, ERL_NIF_TERM term)
{
    double val;
    return enif_get_double(env, term, &val);
}

bool ErlUtil::isInt64()
{
    checkTerm();
    return isInt64(term_);
}

bool ErlUtil::isInt64(ERL_NIF_TERM term)
{
    checkEnv();
    return isInt64(env_, term);
}

bool ErlUtil::isInt64(ErlNifEnv* env, ERL_NIF_TERM term)
{
    ErlNifSInt64 val;
    return enif_get_int64(env, term, &val);
}

bool ErlUtil::isUint64()
{
    checkTerm();
    return isUint64(term_);
}

bool ErlUtil::isUint64(ERL_NIF_TERM term)
{
    checkEnv();
    return isUint64(env_, term);
}

bool ErlUtil::isUint64(ErlNifEnv* env, ERL_NIF_TERM term)
{
    ErlNifUInt64 val;
    return enif_get_uint64(env, term, &val);
}

unsigned ErlUtil::listLength()
{
    checkTerm();
    return listLength(term_);
}

unsigned ErlUtil::listLength(ERL_NIF_TERM term)
{
    checkEnv();
    return listLength(env_, term);
}

unsigned ErlUtil::listLength(ErlNifEnv* env, ERL_NIF_TERM term)
{
    unsigned len=0;

    if(enif_get_list_length(env, term,  &len) == 0) {
        ThrowRuntimeError("Unable to get list length");
    }

    return len;
}

std::string ErlUtil::getAtom()
{
    checkTerm();
    return getAtom(term_);
}

std::string ErlUtil::getAtom(ERL_NIF_TERM term)
{
    checkEnv();
    return getAtom(env_, term);
}

std::string ErlUtil::getAtom(ErlNifEnv* env, ERL_NIF_TERM term, bool toLower)
{
    unsigned len=0;
    if(!enif_get_atom_length(env, term, &len, ERL_NIF_LATIN1))
        ThrowRuntimeError("Unable to encode atom");

    char buf[len+1];
    if(!enif_get_atom(env, term, buf, len+1, ERL_NIF_LATIN1))
        ThrowRuntimeError("Unable to encode atom");

    if(toLower) {
        for(unsigned i=0; i < len; i++)
            buf[i] = tolower(buf[i]);
    }
        
    return buf;
}

std::vector<unsigned char> ErlUtil::getBinary()
{
    checkTerm();
    return getBinary(term_);
}

std::vector<unsigned char> ErlUtil::getBinary(ERL_NIF_TERM term)
{
    checkEnv();
    return getBinary(env_, term);
}

std::vector<unsigned char> ErlUtil::getBinary(ErlNifEnv* env, ERL_NIF_TERM term)
{
    std::vector<unsigned char> ret;

    ErlNifBinary bin;

    if(enif_inspect_binary(env, term, &bin) == 0)
        ThrowRuntimeError("Failed to inspect '" << formatTerm(env, term)
                          << "' as a binary");

    ret.resize(bin.size);
    memcpy(&ret[0], bin.data, bin.size);

    return ret;
}

std::vector<unsigned char> ErlUtil::getAsBinary()
{
    checkTerm();
    return getAsBinary(term_);
}

std::vector<unsigned char> ErlUtil::getAsBinary(ERL_NIF_TERM term)
{
    checkEnv();
    return getAsBinary(env_, term);
}

std::vector<unsigned char> ErlUtil::getAsBinary(ErlNifEnv* env, ERL_NIF_TERM term)
{
    std::vector<unsigned char> ret;

    ErlNifBinary bin;

    bool success = false;
    success = 
        enif_inspect_binary(env, term, &bin) ||
        enif_inspect_iolist_as_binary(env, term, &bin);

    if(!success)
        ThrowRuntimeError("Failed to inspect '" << formatTerm(env, term)
                          << "' as a binary");

    ret.resize(bin.size);
    memcpy(&ret[0], bin.data, bin.size);

    return ret;
}

double ErlUtil::getDouble()
{
    checkTerm();
    return getDouble(term_);
}

double ErlUtil::getDouble(ERL_NIF_TERM term)
{
    checkEnv();
    return getDouble(env_, term);
}

double ErlUtil::getDouble(ErlNifEnv* env, ERL_NIF_TERM term)
{
    double val;
    if(!enif_get_double(env, term, &val))
        ThrowRuntimeError("Term: " << formatTerm(env, term) << " isn't a double");
    return val;
}

int64_t ErlUtil::getInt64()
{
    checkTerm();
    return getInt64(term_);
}

int64_t ErlUtil::getInt64(ERL_NIF_TERM term)
{
    checkEnv();
    return getInt64(env_, term);
}

int64_t ErlUtil::getInt64(ErlNifEnv* env, ERL_NIF_TERM term)
{
    ErlNifSInt64 val;
    if(!enif_get_int64(env, term, &val))
        ThrowRuntimeError("Term: " << formatTerm(env, term) << " isn't an int64");
    return val;
}

uint64_t ErlUtil::getUint64()
{
    checkTerm();
    return getUint64(term_);
}

uint64_t ErlUtil::getUint64(ERL_NIF_TERM term)
{
    checkEnv();
    return getUint64(env_, term);
}

uint64_t ErlUtil::getUint64(ErlNifEnv* env, ERL_NIF_TERM term)
{
    ErlNifUInt64 val;
    if(!enif_get_uint64(env, term, &val))
        ThrowRuntimeError("Term: " << formatTerm(env, term) << " isn't a uint64");
    return val;
}

bool ErlUtil::getBool()
{
    checkTerm();
    return getBool(term_);
}

bool ErlUtil::getBool(ERL_NIF_TERM term)
{
    checkEnv();
    return getBool(env_, term);
}

bool ErlUtil::getBool(ErlNifEnv* env, ERL_NIF_TERM term)
{
    bool val = false;

    if(isAtom(env, term)) {
        std::string atom = getAtom(env, term, true);
        if(atom == "true") {
            val = true;
        } else if(atom == "false") {
            val = false;
        }
    } else {
        ThrowRuntimeError("Term: " << formatTerm(env, term) << " isn't a boolean");
    }

    return val;
}

std::string ErlUtil::getString()
{
    checkTerm();
    return getString(term_);
}

std::string ErlUtil::getString(ERL_NIF_TERM term)
{
    checkEnv();
    return getString(env_, term);
}

std::string ErlUtil::getString(ErlNifEnv* env, ERL_NIF_TERM term)
{
    unsigned size=0;
    std::string retVal;

    //------------------------------------------------------------
    // Atoms are represented as strings
    //------------------------------------------------------------

    if(isAtom(env, term)) {
        return getAtom(env, term);

        //------------------------------------------------------------
        // If this is a binary, it _could_ be a string
        //------------------------------------------------------------

    } else if(isBinary(env, term)) {
        ErlNifBinary bin;

        if(enif_inspect_binary(env, term, &bin) == 0) {
            ThrowRuntimeError("Failed to inspect binary");
        }

        size = bin.size;
        char buf[size+1];

        for(unsigned i=0; i < size; i++) {

            if(isprint_l(bin.data[i], LC_GLOBAL_LOCALE)) {
                buf[i] = bin.data[i];
            } else {
                ThrowRuntimeError("Term '" << formatTerm(env, term) 
                                  << "' is not a printable string");
            }
        }

        buf[size] = '\0';
        return buf;

        //------------------------------------------------------------
        // Erlang represents strings internally as lists, so use native
        // erlang conversion to check a list
        //------------------------------------------------------------

    } else if(isList(env, term)) {

        if(enif_get_list_length(env, term, &size) == 0) {
            ThrowRuntimeError("Failed to get list length");
        }

        char buf[size+1];
        if(enif_get_string(env, term, buf, size+1, ERL_NIF_LATIN1) == 0) {
            ThrowRuntimeError("Unable to encode string");
        }

        return buf;

        //------------------------------------------------------------
        // Else not a string
        //------------------------------------------------------------

    } else {
        ThrowRuntimeError("Term '" << formatTerm(env, term) 
                          << "' cannot be interpreted as a string");
    }
}

/**.......................................................................
 * Return a vector of items from an erlang list
 */
std::vector<ERL_NIF_TERM> ErlUtil::getListCells()
{
    checkTerm();
    return getListCells(term_);
}

std::vector<ERL_NIF_TERM> ErlUtil::getListCells(ERL_NIF_TERM term)
{
    checkEnv();
    return getListCells(env_, term);
}

std::vector<ERL_NIF_TERM> ErlUtil::getListCells(ErlNifEnv* env, ERL_NIF_TERM term)
{
    if(!isList(env, term)) {
        ThrowRuntimeError("Not a list");
    }

    unsigned len = listLength(env, term);

    std::vector<ERL_NIF_TERM> cells(len);
    ERL_NIF_TERM curr, rest = term;

    for(unsigned iCell=0; iCell < len; iCell++) {

        if(enif_get_list_cell(env, rest, &curr, &rest) == 0) {
            ThrowRuntimeError("Unable to get next list cell");
        }

        cells[iCell] = curr;
    }

    return cells;
}

/**.......................................................................
 * Return a vector of items from an erlang tuple
 */
std::vector<ERL_NIF_TERM> ErlUtil::getTupleCells()
{
    checkTerm();
    return getTupleCells(term_);
}

std::vector<ERL_NIF_TERM> ErlUtil::getTupleCells(ERL_NIF_TERM term)
{
    checkEnv();
    return getTupleCells(env_, term);
}

std::vector<ERL_NIF_TERM> ErlUtil::getTupleCells(ErlNifEnv* env, ERL_NIF_TERM term)
{
    if(!isTuple(env, term)) {
        ThrowRuntimeError("Not a tuple");
    }

    int arity=0;
    const ERL_NIF_TERM* array=0;

    if(!enif_get_tuple(env, term, &arity, &array)) 
        ThrowRuntimeError("Unable to parse a tuple");

    std::vector<ERL_NIF_TERM> cells(arity);

    for(int iCell=0; iCell < arity; iCell++)
        cells[iCell] = array[iCell];

    return cells;
}

/**.......................................................................
 * Return a vector of name-value pairs from an erlang list of tuples, ie:
 *
 * [{name, value}, {name, value}, {name, value}]
 */
std::vector<std::pair<std::string, ERL_NIF_TERM> > ErlUtil::getListTuples()
{
    checkTerm();
    return getListTuples(term_);
}

std::vector<std::pair<std::string, ERL_NIF_TERM> > ErlUtil::getListTuples(ERL_NIF_TERM term)
{
    checkEnv();

    if(!isList(term))
        ThrowRuntimeError("Not a list");

    unsigned len = listLength(term);

    std::vector<std::pair<std::string, ERL_NIF_TERM> > cells(len);
    ERL_NIF_TERM curr, rest = term;

    for(unsigned iCell=0; iCell < len; iCell++) {

        if(enif_get_list_cell(env_, rest, &curr, &rest) == 0)
            ThrowRuntimeError("Unable to get next list cell");

        if(!enif_is_tuple(env_, curr))
            ThrowRuntimeError("List cell is not a tuple");

        int arity=0;
        const ERL_NIF_TERM* array=0;
        if(enif_get_tuple(env_, curr, &arity, &array)==0)
            ThrowRuntimeError("Unable to get tuple");
  
        if(arity != 2)
            ThrowRuntimeError("Malformed tuple");

        try {
            cells[iCell].first  = getString(array[0]);
        } catch(std::runtime_error err) {
            std::ostringstream os;
            os << err.what() << " (while processing tuple " << iCell+1 << ")";
            throw std::runtime_error(os.str());
        }

        cells[iCell].second = array[1];
    }

    return cells;
}

void ErlUtil::decodeRiakObject(ERL_NIF_TERM obj, ERL_NIF_TERM encoding)
{
    checkEnv();

    ErlNifBinary bin;
    if(enif_inspect_binary(env_, obj, &bin) == 0)
        ThrowRuntimeError("Term is not a binary");

#if 0
    unsigned encodingLen;
    if(enif_get_atom_length(env_, encoding, &encodingLen, ERL_NIF_LATIN1) == 0)
        ThrowRuntimeError("Term is not an atom");

    char encodingType[encodingLen+1];
    if(enif_get_atom(env_, encoding, encodingType, encodingLen+1, ERL_NIF_LATIN1) == 0)
        ThrowRuntimeError("Term is not an atom");

    COUT("Binary has size: " << bin.size << " Encoding = " << encoding);
#endif

    unsigned char* ptr = bin.data;

    unsigned char magic = (*ptr++);
    COUT("Magic = " << (int)(magic));

    unsigned char vers = (*ptr++);
    COUT("Version = " << (int)(vers));

    unsigned int vClockLen = ntohl(*((unsigned int*)ptr));
    ptr += 4;

    COUT("VclockLen = " << vClockLen);
    ptr += vClockLen;

    unsigned int sibCount =  ntohl(*((unsigned int*)ptr));
    ptr += 4;
    COUT("SibCount = " << sibCount);

    //------------------------------------------------------------
    // Now we are on to the first sibling
    //------------------------------------------------------------

    unsigned int valLen =  ntohl(*((unsigned int*)ptr));
    ptr += 4;
    COUT("ValLen = " << valLen);

    // And the first siblings data

    if(encoding == eleveldb::ATOM_ERLANG_ENCODING)
        parseSiblingData(ptr, valLen);
    else if(encoding == eleveldb::ATOM_MSGPACK_ENCODING)
        parseSiblingDataMsgpack(ptr, valLen);
    else
        ThrowRuntimeError("Unrecognized encoding");
}

void ErlUtil::parseSiblingData(unsigned char* ptr, unsigned len)
{
    for(unsigned i=0; i < len; i++)
        COUT("Byte " << i << " = " << *(ptr+i));
}

void ErlUtil::parseSiblingDataMsgpack(unsigned char* ptr, unsigned len)
{
    cmp_mem_access_t ma;
    uint32_t map_size;
    char key[255];
    cmp_object_t map;
    cmp_ctx_t cmp;
  
    cmp_mem_access_ro_init(&cmp, &ma, ptr, len);
    cmp_read_object(&cmp, &map);
  
    if(!cmp_object_as_map(&map, &map_size)) {
        ThrowRuntimeError("Unable to parse data as a msgpack object");
    }

    //------------------------------------------------------------
    // Iterate over the object, looking for fields
    //------------------------------------------------------------

    for(unsigned int i=0; i < map_size; i++) {

        //------------------------------------------------------------
        // First read the field key
        //------------------------------------------------------------

        cmp_object_t key_obj;

        if (!cmp_read_object(&cmp, &key_obj) || !cmp_object_is_str(&key_obj)) {
            ThrowRuntimeError("Failed to read key");
        }

        if (!cmp_object_to_str(&cmp, &key_obj, key, sizeof(key))) {
            ThrowRuntimeError("Error reading key as string!");
        }

	//------------------------------------------------------------
	// Next read the field value
	//------------------------------------------------------------

        cmp_object_t obj;
        cmp_read_object(&cmp, &obj);

	if (cmp_object_is_nil(&obj)) {
            continue;

	} else if (cmp_object_is_long(&obj)) {
            int64_t val;
            if (cmp_object_as_long(&obj, &val)) {
                COUT("Found field " << key << " with val (1) " << val);
            }
	} else if (cmp_object_is_ulong(&obj)) {
            uint64_t val;
            if (cmp_object_as_ulong(&obj, &val)) {
                COUT("Found field " << key << " with val (2) " << val);
            }
	} else if (cmp_object_is_float(&obj)) {
            float val;
            if (cmp_object_as_float(&obj, &val)) {
                COUT("Found field " << key << " with val (2) " << val);
            }
	} else if (cmp_object_is_double(&obj)) {
            double val;
            if (cmp_object_as_double(&obj, &val)) {
                COUT("Found field " << key << " with val (2) " << val);
            }
	} else {
            ThrowRuntimeError("Unsupported value type found!");
	}
    }
}

/**.......................................................................
 * Try to convert the erlang value in term to an integer
 */
int32_t ErlUtil::getValAsInt32(ErlNifEnv* env, ERL_NIF_TERM term, bool exact)
{
    // Integers can be converted to integers

    int intVal;
    if(enif_get_int(env, term, &intVal))
        return intVal;

    // Unsigned integers can be converted to integers, as long as they
    // are not greater than INT_MAX

    unsigned int uintVal;
    if(enif_get_uint(env, term, &uintVal))
        if(uintVal <= INT_MAX)
            return (int)uintVal;

    // int64_ts can be converted to integers, as long as they
    // are within the representable range of ints

    ErlNifSInt64 int64Val;
    if(enif_get_int64(env, term, &int64Val))
        if(int64Val <= INT_MAX && int64Val >= INT_MIN)
            return (int)int64Val;

    // uint64_ts can be converted to integers, as long as they
    // are not greater than INT_MAX

    ErlNifUInt64 uint64Val;
    if(enif_get_uint64(env, term, &uint64Val))
        if(uint64Val <= INT_MAX)
            return (int)uint64Val;

    // Doubles can be converted to ints, as long as they are within the
    // representable range of ints and have no fractional component

    double doubleVal;
    if(enif_get_double(env, term, &doubleVal)) {

        if(doubleVal <= (double)INT_MAX && doubleVal >= (double)INT_MIN) {
            if(!exact || !(fabs(doubleVal - (int)doubleVal) > 0.0))
                return (int)doubleVal;
        }
    } 

    //------------------------------------------------------------
    // Atoms can be represented if they are boolean values
    //------------------------------------------------------------

    if(ErlUtil::isAtom(env, term)) {
        std::string atom = ErlUtil::getAtom(env, term, true);
        if(atom == "true") {
            return 1;
        } else if(atom == "false") {
            return 0;
        }
    }

    ThrowRuntimeError("Erlang value " << formatTerm(env, term) 
                      << " can't be represented as an integer");
    return 0;
}

/**.......................................................................
 * Try to convert the erlang value in term to a 64-bit integer
 */
int64_t ErlUtil::getValAsInt64(ErlNifEnv* env, ERL_NIF_TERM term, bool exact)
{
    // int64_ts can always be converted to 64-bit integers

    ErlNifSInt64 int64Val;
    if(enif_get_int64(env, term, &int64Val))
        return int64Val;

    // Integers can always be converted to 64-bit integers

    int intVal;
    if(enif_get_int(env, term, &intVal))
        return intVal;

    // Unsigned integers can always be converted to 64-bit integers

    unsigned int uintVal;
    if(enif_get_uint(env, term, &uintVal))
        return (int64_t)uintVal;

    // uint64_ts can be converted to 64-bit integers, as long as they
    // are not greater than LLONG_MAX

    ErlNifUInt64 uint64Val;
    if(enif_get_uint64(env, term, &uint64Val))
        if(uint64Val <= LLONG_MAX)
            return (int64_t)uint64Val;

    // Doubles can be converted to 64-bit ints, as long as they are
    // within the representable range of 64-bit ints and have no fractional
    // component

    double doubleVal;
    if(enif_get_double(env, term, &doubleVal)) {

        if(doubleVal <= (double)LLONG_MAX && doubleVal >= (double)LLONG_MIN) {
            if(!exact || !(fabs(doubleVal - (int)doubleVal) > 0.0))
                return (int64_t)doubleVal;
        }
    } 

    //------------------------------------------------------------
    // Atoms can be represented if they are boolean values
    //------------------------------------------------------------

    if(ErlUtil::isAtom(env, term)) {
        std::string atom = ErlUtil::getAtom(env, term, true);
        if(atom == "true") {
            return 1;
        } else if(atom == "false") {
            return 0;
        }
    }

    ThrowRuntimeError("Erlang value " << formatTerm(env, term) 
                      << " can't be represented as an int64_t");
    return 0;
}

/**.......................................................................
 * Try to convert the erlang value in term to an unsigned integer
 */
uint32_t ErlUtil::getValAsUint32(ErlNifEnv* env, ERL_NIF_TERM term, bool exact)
{
    //------------------------------------------------------------
    // Unsigned integers can be converted to unsigned integers
    //------------------------------------------------------------

    unsigned int uintVal;
    if(enif_get_uint(env, term, &uintVal))
        return uintVal;

    //------------------------------------------------------------
    // Integers can be converted to unsigned integers, as long as they
    // as positive
    //------------------------------------------------------------
    
    int intVal;
    if(enif_get_int(env, term, &intVal)) {
        if(intVal >= 0)
            return (unsigned int)intVal;
    }

    //------------------------------------------------------------
    // int64_t can be converted to integers, as long as they are
    // positive, and within the representable range of unsigned ints
    //------------------------------------------------------------

    ErlNifSInt64 int64Val;
    if(enif_get_int64(env, term, &int64Val))
        if(int64Val >= 0 && int64Val <= UINT_MAX)
            return (unsigned int)int64Val;

    //------------------------------------------------------------
    // uint64_ts can be converted to integers, as long as they
    // are not greater than UINT_MAX
    //------------------------------------------------------------

    ErlNifUInt64 uint64Val;
    if(enif_get_uint64(env, term, &uint64Val))
        if(uint64Val <= UINT_MAX)
            return (unsigned int)uint64Val;

    //------------------------------------------------------------
    // Doubles can be converted to unsigned ints, as long as they are
    // within the representable range of unsigned ints and have no
    // fractional component
    //------------------------------------------------------------

    double doubleVal;
    if(enif_get_double(env, term, &doubleVal)) {
        if(doubleVal >= 0.0) {
            if(doubleVal <= (double)UINT_MAX) {
                if(!exact || !(fabs(doubleVal - (int)doubleVal) > 0.0))
                    return (int)doubleVal;
            }
        }
    } 

    //------------------------------------------------------------
    // Atoms can be represented if they are boolean values
    //------------------------------------------------------------

    if(ErlUtil::isAtom(env, term)) {
        std::string atom = ErlUtil::getAtom(env, term, true);
        if(atom == "true") {
            return 1;
        } else if(atom == "false") {
            return 0;
        }
    }

    ThrowRuntimeError("Erlang value " << formatTerm(env, term) 
                      << " can't be represented as an unsigned integer");

    return 0;
}

/**.......................................................................
 * Try to convert the erlang value in term to a uint8_t
 */
uint8_t ErlUtil::getValAsUint8(ErlNifEnv* env, ERL_NIF_TERM term, bool exact)
{
    //------------------------------------------------------------
    // Unsigned integers can be converted to uint8_t, as long as they
    // don't exceed UCHAR_MAX
    //------------------------------------------------------------

    unsigned int uintVal;
    if(enif_get_uint(env, term, &uintVal))
        if(uintVal <= UCHAR_MAX)
            return uintVal;

    //------------------------------------------------------------
    // Integers can be converted to uint8_t, as long as they
    // as positive and don't exceed UCHAR_MAX
    //------------------------------------------------------------
    
    int intVal;
    if(enif_get_int(env, term, &intVal)) {
        if(intVal >= 0 && intVal <= UCHAR_MAX)
           return intVal;
    }

    //------------------------------------------------------------
    // int64_t can be converted to integers, as long as they are
    // positive, and within the representable range of uint8_t
    //------------------------------------------------------------

    ErlNifSInt64 int64Val;
    if(enif_get_int64(env, term, &int64Val))
        if(int64Val >= 0 && int64Val <= UCHAR_MAX)
            return int64Val;

    //------------------------------------------------------------
    // uint64_ts can be converted to integers, as long as they
    // are not greater than UCHAR_MAX
    //------------------------------------------------------------

    ErlNifUInt64 uint64Val;
    if(enif_get_uint64(env, term, &uint64Val))
        if(uint64Val <= UCHAR_MAX)
            return uint64Val;

    //------------------------------------------------------------
    // Doubles can be converted to unsigned ints, as long as they are
    // within the representable range of uint8_t and have no
    // fractional component
    //------------------------------------------------------------

    double doubleVal;
    if(enif_get_double(env, term, &doubleVal)) {
        if(doubleVal >= 0.0) {
            if(doubleVal <= (double)UCHAR_MAX) {
                if(!exact || !(fabs(doubleVal - (int)doubleVal) > 0.0))
                    return doubleVal;
            }
        }
    } 

    //------------------------------------------------------------
    // Atoms can be represented as uint8_t if they are boolean values
    //------------------------------------------------------------

    if(ErlUtil::isAtom(env, term)) {
        std::string atom = ErlUtil::getAtom(env, term, true);
        if(atom == "true") {
            return 1;
        } else if(atom == "false") {
            return 0;
        }
    }

    ThrowRuntimeError("Erlang value " << formatTerm(env, term) 
                      << " can't be represented as a uint8_t");

    return 0;
}

/**.......................................................................
 * Try to convert the erlang value in term to a 64-bit unsigned integer
 */
uint64_t ErlUtil::getValAsUint64(ErlNifEnv* env, ERL_NIF_TERM term, bool exact)
{
    //------------------------------------------------------------
    // uint64_t can be converted to 64-bit integers
    //------------------------------------------------------------

    ErlNifUInt64 uint64Val;
    if(enif_get_uint64(env, term, &uint64Val))
        return uint64Val;

    //------------------------------------------------------------
    // Integers can be converted to 64-bit unsigned integers, as long as
    // they as positive
    //------------------------------------------------------------

    int intVal;
    if(enif_get_int(env, term, &intVal)) {
        if(intVal >= 0)
            return (uint64_t)intVal;
    }

    //------------------------------------------------------------
    // Unsigned integers can be converted to 64-bit unsigned integers
    //------------------------------------------------------------

    unsigned int uintVal;
    if(enif_get_uint(env, term, &uintVal))
        return (uint64_t)uintVal;

    //------------------------------------------------------------
    // int64_t can be converted to integers, as long as they are
    // positive
    //------------------------------------------------------------

    ErlNifSInt64 int64Val;
    if(enif_get_int64(env, term, &int64Val))
        if(int64Val >= 0)
            return (uint64_t)int64Val;

    //------------------------------------------------------------
    // Doubles can be converted to 64-bit unsigned ints, as long as they
    // are within the representable range of 64-bit unsigned ints and
    // have no fractional component
    //------------------------------------------------------------

    double doubleVal;
    if(enif_get_double(env, term, &doubleVal)) {
        if(doubleVal >= 0.0) {
            if(doubleVal <= (double)ULLONG_MAX) {
                if(!exact || !(fabs(doubleVal - (int)doubleVal) > 0.0))
                    return (int)doubleVal;
            }
        }
    } 

    //------------------------------------------------------------
    // Atoms can be represented if they are boolean values
    //------------------------------------------------------------

    if(ErlUtil::isAtom(env, term)) {
        std::string atom = ErlUtil::getAtom(env, term, true);
        if(atom == "true") {
            return 1;
        } else if(atom == "false") {
            return 0;
        }
    }

    ThrowRuntimeError("Erlang value " << formatTerm(env, term) 
                      << " can't be represented as a uint64_t");

    return 0;
}

/**.......................................................................
 * Try to convert the erlang value in term to a 64-bit unsigned integer
 */
double ErlUtil::getValAsDouble(ErlNifEnv* env, ERL_NIF_TERM term, bool exact)
{
    double doubleVal;
    if(enif_get_double(env, term, &doubleVal))
        return doubleVal;

    int intVal;
    if(enif_get_int(env, term, &intVal)) {
        return (double)intVal;
    }

    unsigned int uintVal;
    if(enif_get_uint(env, term, &uintVal))
        return (double)uintVal;

    ErlNifSInt64 int64Val;
    if(enif_get_int64(env, term, &int64Val))
        return (double)int64Val;

    ErlNifUInt64 uint64Val;
    if(enif_get_uint64(env, term, &uint64Val))
        return (double)uint64Val;

    //------------------------------------------------------------
    // Atoms can be represented if they are boolean values
    //------------------------------------------------------------

    if(ErlUtil::isAtom(env, term)) {
        std::string atom = ErlUtil::getAtom(env, term, true);
        if(atom == "true") {
            return 1.0;
        } else if(atom == "false") {
            return 0.0;
        }
    }

    ThrowRuntimeError("Erlang value " << formatTerm(env, term) 
                      << " can't be represented as a double");

    return 0.0;
}

/**.......................................................................
 * Format an erlang value
 */
std::string ErlUtil::formatTerm()
{
    checkTerm();
    return formatTerm(term_);
}

std::string ErlUtil::formatTerm(ERL_NIF_TERM term)
{
    checkEnv();
    return formatTerm(env_, term);
}

std::string ErlUtil::formatTerm(ErlNifEnv* env, ERL_NIF_TERM term)
{
    if(isAtom(env, term)) {
        return formatAtom(env, term);
    }

    if(isNumber(env, term)) {
        return formatNumber(env, term);
    }

    if(isTuple(env, term)) {
        return formatTuple(env, term);
    }

    if(isBinary(env, term)) {
        return formatBinary(env, term);
    }

    if(isString(env, term)) {
        return formatString(env, term);
    }

    if(isList(env, term)) {
        return formatList(env, term);
    }

    return "?";
}

std::string ErlUtil::formatAtom(ErlNifEnv* env, ERL_NIF_TERM term)
{
    std::ostringstream os;
    os << getAtom(env, term);

    return os.str();
}

std::string ErlUtil::formatNumber(ErlNifEnv* env, ERL_NIF_TERM term)
{
    std::ostringstream os;

    int intVal;
    if(enif_get_int(env, term, &intVal)) {
        os << intVal;
        return os.str();
    }

    unsigned int uintVal;
    if(enif_get_uint(env, term, &uintVal)) {
        os << uintVal;
        return os.str();
    }

    ErlNifSInt64 int64Val;
    if(enif_get_int64(env, term, &int64Val)) {
        os << int64Val;
        return os.str();
    }

    ErlNifUInt64 uint64Val;
    if(enif_get_uint64(env, term, &uint64Val)) {
        os << uint64Val;
        return os.str();
    }

    double doubleVal;
    if(enif_get_double(env, term, &doubleVal)) {
        os << doubleVal;
        return os.str();
    }

    return "?";
}

std::string ErlUtil::formatString(ErlNifEnv* env, ERL_NIF_TERM term)
{
    std::ostringstream os;
    std::string str = getString(env, term);
    os << "\"" << str << "\"";
    os << " (aka [";
    for(unsigned i=0; i < str.size(); i++) {
        os << (int)str[i];
        if(i < str.size()-1)
            os << ", ";
    }
    os << "])";

    return os.str();
}

std::string ErlUtil::formatList(ErlNifEnv* env, ERL_NIF_TERM term)
{
    std::vector<ERL_NIF_TERM> cells = getListCells(env, term);
    std::ostringstream os;

    os << "[";
    for(unsigned iCell=0; iCell < cells.size(); iCell++) {
        os << formatTerm(env, cells[iCell]);
        if(iCell < cells.size()-1)
            os << ", ";
    }
    os << "]";

    return os.str();
}

std::string ErlUtil::formatBinary(ErlNifEnv* env, ERL_NIF_TERM term)
{
    ErlNifBinary bin;
    if(enif_inspect_binary(env, term, &bin) == 0)
        ThrowRuntimeError("Failed to inspect binary");

    std::ostringstream os;

    os << "<<";
    for(unsigned iByte=0; iByte < bin.size; iByte++) {
        os << (int)bin.data[iByte];
        if(iByte < bin.size-1)
            os << ", ";
    }
    os << ">>";

    return os.str();
}

std::string ErlUtil::formatBinary(const std::string& str)
{
    return formatBinary((std::string&) str);
}

std::string ErlUtil::formatBinary(std::string& str)
{
    return formatBinary(&str[0], str.size());
}

std::string ErlUtil::formatBinary(unsigned char* data, size_t size)
{
    return formatBinary((char*)data, size);
}

std::string ErlUtil::formatBinary(const unsigned char* data, size_t size)
{
    return formatBinary((char*)data, size);
}

std::string ErlUtil::formatBinary(const char* data, size_t size)
{
    return formatBinary((char*)data, size);
}

std::string ErlUtil::formatBinary(char* data, size_t size)
{
    std::ostringstream os;

    os << "<<";
    for(unsigned iByte=0; iByte < size; iByte++) {
        unsigned char c = data[iByte];
        os << (unsigned int)c  << " (" << (char)data[iByte] << ")";
        if(iByte < size-1)
            os << ", ";
    }
    os << ">>";

    return os.str();
}

std::string ErlUtil::formatTupleVec(ErlNifEnv* env, std::vector<ERL_NIF_TERM>& cells)
{
    std::ostringstream os;
    
    os << "{";
    for(unsigned iCell=0; iCell < cells.size(); iCell++) {
        os << formatTerm(env, cells[iCell]);
        if(iCell < cells.size()-1)
            os << ", ";
    }

    os << "}";

    return os.str();
}

std::string ErlUtil::formatTuple(ErlNifEnv* env, ERL_NIF_TERM term)
{
    std::vector<ERL_NIF_TERM> cells = getTupleCells(env, term);
    return formatTupleVec(env, cells);
}
