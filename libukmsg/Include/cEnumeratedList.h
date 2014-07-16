#ifndef CENUMERATEDLIST_H
#define	CENUMERATEDLIST_H

#include <stddef.h>

template <class T>
class cListNode
{
public:
    T* mObject;
    cListNode<T> * mNext;
};

template <class T>
class cEnumeratedList
{
public:
    cEnumeratedList();
    cEnumeratedList(const cEnumeratedList& orig);
    virtual ~cEnumeratedList();
    int insert(T*);
    int insertBack(T*);
    T* first();
    T* getNext();
    void resetEnum();
    int remove(T*);
    void link(cEnumeratedList<T> *);
    void unlink();
private:
    cListNode<T> * mHead;
    cListNode<T> * mCurrEnum;
    cEnumeratedList<T> * mLinkedList;

};


template <class T> cEnumeratedList<T>::cEnumeratedList()
{
    mHead = NULL;
    mCurrEnum = mHead;
    mLinkedList = NULL;
}

template <class T> cEnumeratedList<T>::cEnumeratedList(const cEnumeratedList<T>& orig)
{

}

template <class T>cEnumeratedList<T>::~cEnumeratedList()
{
    cListNode<T> * d;

    while(mHead)
    {
        d = mHead;
        mHead = mHead->mNext;
        delete d;
    }
}

template <class T> int cEnumeratedList<T>::insert(T* obj)
{
    cListNode<T> * n = new cListNode<T>();

    n->mObject = obj;
    n->mNext = mHead;
    mHead = n;

    if(mLinkedList)
    {
        mLinkedList->insert(obj);
    }

    return 0;
}

template <class T> int cEnumeratedList<T>::insertBack(T* obj)
{
     cListNode<T> * n = new cListNode<T>();
     cListNode<T> * idx = mHead;
     n->mObject = obj;
     if(idx)
     {
         while(idx->mNext)
         {
             idx = idx->mNext;
         }
         idx->mNext = n;
         n->mNext = NULL;
     }
     else
     {
         mHead = n;
         n->mNext = NULL;
     }
     return 0;
}

template <class T> int cEnumeratedList<T>::remove(T* obj)
{
    int stat = -1;
    cListNode<T> * idx = mHead;
    cListNode<T> * rem = NULL;

    if(idx->mObject == obj)
    {
       mHead = idx->mNext;
       rem = idx;
    }
    else
    {
       while(idx->mNext)
       {
          rem = idx->mNext;
          if(rem->mObject == obj)
          {
             idx->mNext = rem->mNext;
             break;
          }
          idx = idx->mNext;
       }
    }

    if(rem)
    {
        if(rem == mCurrEnum)
        {
            mCurrEnum = rem->mNext;
        }
        delete rem;
        stat = 0;
    }

    if(mLinkedList)
    {
        mLinkedList->remove(obj);
    }

    return stat;
}

template <class T> T* cEnumeratedList<T>::first()
{
    T* r = NULL;
    if(mHead)
    {
        r = mHead->mObject;
       mCurrEnum = mHead->mNext;
    }
    else
    {
        mCurrEnum = mHead;
    }

    return r;
}

template <class T> T* cEnumeratedList<T>::getNext()
{
    T* r = NULL;
    if(mCurrEnum)
    {
        r = mCurrEnum->mObject;
        mCurrEnum = mCurrEnum->mNext;
    }
    else
    {
        r = NULL;
        mCurrEnum = mHead;
    }
    return r;
}

template <class T> void cEnumeratedList<T>::link(cEnumeratedList<T> *l)
{
    mLinkedList = l;
}

template <class T> void cEnumeratedList<T>::unlink()
{
    mLinkedList = NULL;
}
#endif
