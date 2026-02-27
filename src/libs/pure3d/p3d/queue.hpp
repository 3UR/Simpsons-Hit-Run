//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _QUEUE_HPP_
#define _QUEUE_HPP_

#include <p3d/error.hpp>
#include <p3d/utility.hpp>
#include <limits.h>

template <class T> class tQueue
{
public:
    tQueue(unsigned s) :
        buffer(new T[s]),
        size(s),
        head(0),
        tail(0)
    {
    }

    ~tQueue()
    {
        delete buffer;
    }

    bool Empty(void)
    {
        return head == tail;
    }

    // add an element to the queue
    void Enqueue(T t)
    {
        unsigned current = head;
        head = Next(head);
        P3DASSERT(head != tail); // we've eaten our own tail
        buffer[current] = t;
    }

    // remove next element from the queue
    T Dequeue(void)
    {
        P3DASSERT( head != tail); // when head and tail are same 
        unsigned current = tail;
        tail = Next(tail);
        return buffer[current];
    }

protected:
    // get the next index in ring buffer
    unsigned Next(unsigned v)
    {
        v++;
        if(v == size) return 0;
        return v;
    }

    // ring buffer
    T* buffer;
    unsigned size; // size of buffer
    unsigned head; // location to add a newly enqueded element
    unsigned tail;  // location to remove next a dequeued element from

};

template <class T> class tPriorityQueue
{
public:
    tPriorityQueue(unsigned s) :
        buffer(new Element[s]),
        size(s),
        nextFree(0)
    {
        for(unsigned i = 0; i < size; i++)
        {
            buffer[i].priority = INT_MIN;
        }
    }

    ~tPriorityQueue()
    {
        delete buffer;
    }

    bool Empty(void)
    {
        return nextFree == 0;
    }

    // add an element to the queue
    void Enqueue(T t, int priority = 0)
    {
        P3DASSERT(priority != INT_MIN);
        P3DASSERT(nextFree != size);
        buffer[nextFree].priority = priority;
        buffer[nextFree].t = t;
        TrickleUp(nextFree);
        nextFree++;
    }

    // remove next element from the queue
    T Dequeue(void)
    {
        P3DASSERT(nextFree > 0);
        T t = buffer[0].t;
        nextFree--;
        if(nextFree != 0)
        {
            buffer[0] = buffer[nextFree];
            TrickleDown(0);
        }
        buffer[nextFree].priority = INT_MIN;
        return t;
    }

protected:
    void Swap(int a, int b)
    {
        Element tmp = buffer[a];
        buffer[a] = buffer[b];
        buffer[b] = tmp;
    }

    int Parent(unsigned i) {
        return ((i+1)/2)-1;
    }

    int LeftChild(unsigned i) {
        return ((i+1)*2)-1;
    }

    int RightChild(unsigned i) {
        return (((i+1)*2)+1)-1;
    }

    void TrickleUp(unsigned node)
    {
        if(node == 0)
            return;

        if(buffer[node].priority > buffer[Parent(node)].priority)
        {
            Swap(node, Parent(node));
            TrickleUp(Parent(node));
        }
    }

    void TrickleDown(unsigned node)
    {
        if(node >= nextFree)
            return;

        if((buffer[node].priority > buffer[LeftChild(node)].priority) && (buffer[node].priority > buffer[RightChild(node)].priority))
            return;

        if(buffer[LeftChild(node)].priority > buffer[RightChild(node)].priority)
        {
            Swap(node, LeftChild(node));
            TrickleDown(LeftChild(node));
        }
        else
        {
            Swap(node, RightChild(node));
            TrickleDown(RightChild(node));
        }
    }




    struct Element
    {
        int priority;
        T t;
    };

    Element* buffer;
    unsigned size;
    unsigned nextFree;
};

template <class T> class tOrderedPriorityQueue : public tPriorityQueue<T>
{
public:
    tOrderedPriorityQueue(unsigned s) : tPriorityQueue<T>(s), sequenceNumber(SHRT_MAX)
    {
    }

    // add an element to the queue
    void Enqueue(T t, int priority = 0)
    {
        P3DASSERT((priority >= SHRT_MIN) && (priority <= SHRT_MAX));
        tPriorityQueue<T>::Enqueue(t, (priority << 16) + sequenceNumber);

        if(sequenceNumber == 0)
        {
            p3d::print("warning : sequence number wrapped in tOrderedPriorityQuere, bad ordering may result\n");
            sequenceNumber = SHRT_MAX;
        }
        else
        {
            sequenceNumber--;
        }
    }

    // remove next element from the queue
    T Dequeue(void)
    {
        if (this->Empty())
            sequenceNumber = SHRT_MAX;
        return tPriorityQueue<T>::Dequeue();
    }

protected:
    unsigned sequenceNumber;
};

#endif
