//
// eleveldb: Erlang Wrapper for LevelDB (http://code.google.com/p/leveldb/)
//
// Copyright (c) 2011-2015 Basho Technologies, Inc. All Rights Reserved.
//
// This file is provided to you under the Apache License,
// Version 2.0 (the "License"); you may not use this file
// except in compliance with the License.  You may obtain
// a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// -------------------------------------------------------------------

#ifndef INCL_REFOBJECTS_H
#define INCL_REFOBJECTS_H

#include <stdint.h>
#include <sys/time.h>
#include <list>

#include "leveldb/db.h"
#include "leveldb/write_batch.h"
#include "leveldb/perf_count.h"
#include "util/refobject_base.h"
#define LEVELDB_PLATFORM_POSIX
#include "port/port.h"

#ifndef __WORK_RESULT_HPP
    #include "work_result.hpp"
#endif

#ifndef ATOMS_H
    #include "atoms.h"
#endif


namespace eleveldb {

/**
 * Base class for any object that offers RefInc / RefDec interface
 *  Today the class' sole purpose is to provide eleveldb specific
 *  performance counters.
 */
class RefObject : public leveldb::RefObjectBase
{
public:
    RefObject();

    virtual ~RefObject();

private:
    RefObject(const RefObject&);              // nocopy
    RefObject& operator=(const RefObject&);   // nocopyassign
};  // class RefObject


/**
 * Base class for any object that is managed as an Erlang reference
 */

class ErlRefObject : public RefObject
{
public:
    // Pointer to "this" from within Erlang heap.
    // First thread to clear "this" in Erlang heap
    //  owns the shutdown (Erlang or async C)
    void * volatile * m_ErlangThisPtr;

    leveldb::port::Mutex   m_CloseMutex;        //!< for condition wait
    leveldb::port::CondVar m_CloseCond;         //!< for notification of user's finish

private:  // private to force use of GetCloseRequest()
    // m_CloseRequested assumes m_CloseMutex held for writes
    // 1 once InitiateCloseRequest starts,
    // 2 other pointers to "this" released
    // 3 final RefDec and destructor executing
    volatile uint32_t m_CloseRequested;

public:
    ErlRefObject();

    virtual ~ErlRefObject();

    virtual uint32_t RefDec();

    virtual void Shutdown()=0;

    bool ClaimCloseFromCThread();

    void InitiateCloseRequest();

    // memory fencing for reads
    uint32_t GetCloseRequested()
        {return(leveldb::add_and_fetch(&m_CloseRequested, (uint32_t)0));};

private:
    ErlRefObject(const ErlRefObject&);              // nocopy
    ErlRefObject& operator=(const ErlRefObject&);   // nocopyassign
};  // class RefObject



/**
 * Class to manage access and counting of references
 * to a reference object.
 */

template <class TargetT>
class ReferencePtr
{
    TargetT * t;

public:
    ReferencePtr()
        : t(NULL)
    {};

    ReferencePtr(TargetT *_t)
        : t(_t)
    {
        if (NULL!=t)
            t->RefInc();
    }

    ReferencePtr(const ReferencePtr &rhs)
    {t=rhs.t; if (NULL!=t) t->RefInc();};

    ~ReferencePtr()
    {
        TargetT * temp_ptr(t);
        t=NULL;
        if (NULL!=temp_ptr)
            temp_ptr->RefDec();
    }

    void assign(TargetT * _t)
    {
        if (_t!=t)
        {
            if (NULL!=t)
                t->RefDec();
            t=_t;
            if (NULL!=t)
                t->RefInc();
        }   // if
    };

    TargetT * get() {return(t);};

    TargetT * operator->() {return(t);};

private:
 ReferencePtr & operator=(const ReferencePtr & rhs); // no assignment

};  // ReferencePtr


/**
 * Per database object.  Created as erlang reference.
 *
 * Extra reference count created upon initialization, released on close.
 */
class DbObject : public ErlRefObject
{
public:
    leveldb::DB* m_Db;                                   // NULL or leveldb database object

    leveldb::Options * m_DbOptions;

    leveldb::port::Mutex m_ItrMutex;                         //!< mutex protecting m_ItrList
    std::list<class ItrObject *> m_ItrList;   //!< ItrObjects holding ref count to this

protected:
    static ErlNifResourceType* m_Db_RESOURCE;

public:
    DbObject(leveldb::DB * DbPtr, leveldb::Options * Options);

    virtual ~DbObject();

    virtual void Shutdown();

    // manual back link to ItrObjects holding reference to this
    bool AddReference(class ItrObject *);

    void RemoveReference(class ItrObject *);

    static void CreateDbObjectType(ErlNifEnv * Env);

    static void * CreateDbObject(leveldb::DB * Db, leveldb::Options * DbOptions);

    static DbObject * RetrieveDbObject(ErlNifEnv * Env, const ERL_NIF_TERM & DbTerm, bool * term_ok=NULL);

    static void DbObjectResourceCleanup(ErlNifEnv *Env, void * Arg);

private:
    DbObject();
    DbObject(const DbObject&);              // nocopy
    DbObject& operator=(const DbObject&);   // nocopyassign
};  // class DbObject


/**
 * A self deleting wrapper to contain leveldb iterator.
 *   Used when an ItrObject needs to skip around and might
 *   have a background MoveItem performing a prefetch on existing
 *   iterator.
 */

class LevelIteratorWrapper : public RefObject
{
public:
    ReferencePtr<DbObject> m_DbPtr;           //!< need to keep db open for delete of this object
    ReferencePtr<class ItrObject> m_ItrPtr;         //!< shared itr_ref requires we hold ItrObject
    const leveldb::Snapshot * m_Snapshot;
    leveldb::Iterator * m_Iterator;
    volatile uint32_t m_HandoffAtomic;        //!< matthew's atomic foreground/background prefetch flag.
    bool m_KeysOnly;                          //!< only return key values
    // m_PrefetchStarted must use uint32_t instead of bool for Solaris CAS operations
    volatile uint32_t m_PrefetchStarted;          //!< true after first prefetch command
    leveldb::ReadOptions m_Options;           //!< local copy of ItrObject::options
    ERL_NIF_TERM itr_ref;                     //!< shared copy of ItrObject::itr_ref

    // only used if m_Options.iterator_refresh == true
    std::string m_RecentKey;                  //!< Most recent key returned
    time_t m_IteratorStale;                   //!< time iterator should refresh
    bool m_StillUse;                          //!< true if no error or key end seen

    // debug data for hung iteratos
    time_t m_IteratorCreated;                 //!< time constructor called
    time_t m_LastLogReport;                   //!< LOG message was last written
    size_t m_MoveCount;                       //!< number of calls to MoveItem

    // read by Erlang thread, maintained by eleveldb MoveItem::DoWork
    volatile bool m_IsValid;                  //!< iterator state after last operation

    LevelIteratorWrapper(ItrObject * ItrPtr, bool KeysOnly,
                         leveldb::ReadOptions & Options, ERL_NIF_TERM itr_ref);

    virtual ~LevelIteratorWrapper()
    {
        PurgeIterator();
    }   // ~LevelIteratorWrapper

    leveldb::Iterator * get() {return(m_Iterator);};

    volatile bool Valid() {return(m_IsValid);};
    void SetValid(bool flag) {m_IsValid=flag;};

    // warning, only valid with Valid() otherwise potential
    //   segfault if iterator purged to fix AAE'ism
    leveldb::Slice key() {return(m_Iterator->key());};
    leveldb::Slice value() {return(m_Iterator->value());};

    // iterator_refresh related routines
    void PurgeIterator()
    {
        if (NULL!=m_Snapshot)
        {
            const leveldb::Snapshot * temp_snap(m_Snapshot);

            m_Snapshot=NULL;
            // leveldb performs actual "delete" call on m_Shapshot's pointer
            m_DbPtr->m_Db->ReleaseSnapshot(temp_snap);
        }   // if

        if (NULL!=m_Iterator)
        {
            leveldb::Iterator * temp_iter(m_Iterator);

            m_Iterator=NULL;
            delete temp_iter;
        }   // if
    }   // PurgeIterator

    void RebuildIterator()
    {
        struct timeval tv;

        gettimeofday(&tv, NULL);
        m_IteratorStale=tv.tv_sec + 300; // +5min

        PurgeIterator();
        m_Snapshot = m_DbPtr->m_Db->GetSnapshot();
        m_Options.snapshot = m_Snapshot;
        m_Iterator = m_DbPtr->m_Db->NewIterator(m_Options);
    }   // RebuildIterator

    // hung iterator debug
    void LogIterator();

private:
    LevelIteratorWrapper(const LevelIteratorWrapper &);            // no copy
    LevelIteratorWrapper& operator=(const LevelIteratorWrapper &); // no assignment

};  // LevelIteratorWrapper



/**
 * Per Iterator object.  Created as erlang reference.
 */
class ItrObject : public ErlRefObject
{
public:
    ReferencePtr<LevelIteratorWrapper> m_Iter;

    bool keys_only;
    leveldb::ReadOptions m_ReadOptions; //!< local copy, pass to LevelIteratorWrapper only

    volatile class MoveTask * reuse_move;  //!< iterator work object that is reused instead of lots malloc/free

    ReferencePtr<DbObject> m_DbPtr;

    ERL_NIF_TERM itr_ref;                  //!< what was caller ref to async_iterator
    ErlNifEnv *itr_ref_env;                //!< Erlang Env to hold itr_ref


protected:
    static ErlNifResourceType* m_Itr_RESOURCE;

public:
    ItrObject(DbObject *, bool, leveldb::ReadOptions &);

    virtual ~ItrObject(); // needs to perform free_itr

    virtual void Shutdown();

    static void CreateItrObjectType(ErlNifEnv * Env);

    static void * CreateItrObject(DbObject * Db, bool KeysOnly, leveldb::ReadOptions & Options);

    static ItrObject * RetrieveItrObject(ErlNifEnv * Env, const ERL_NIF_TERM & DbTerm,
                                         bool ItrClosing=false);

    static void ItrObjectResourceCleanup(ErlNifEnv *Env, void * Arg);

    bool ReleaseReuseMove();

private:
    ItrObject();
    ItrObject(const ItrObject &);            // no copy
    ItrObject & operator=(const ItrObject &); // no assignment

};  // class ItrObject

} // namespace eleveldb


#endif  // INCL_REFOBJECTS_H
