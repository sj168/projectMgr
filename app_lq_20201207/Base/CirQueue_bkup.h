
#ifndef CIRQUEUE_H
#define CIRQUEUE_H

#include <pthread.h>
#include <stdio.h>


template<typename T>
class CCirQueue
{
public:
    CCirQueue(int iLen);
    ~CCirQueue();
    int cirQueueIn(const T & t);
    T & cirQueueGet(bool * pbOk);
    int cirQueueOut(T & t);
    T & cirQueueOut(bool * pbOk);
    bool isEmpty();
    bool isFull();
    unsigned int length();
    void reset();

private:
    int m_iHead;
    int m_iTail;
    int m_iLen;
    T * m_pT;
    pthread_mutex_t m_Mutex;
};

template<typename T>
CCirQueue<T>::CCirQueue(int iLen) :
    m_iHead(0),
    m_iTail(0),
    m_iLen(iLen),
    m_pT(NULL)
{
    pthread_mutex_init(&m_Mutex, NULL);

    if (iLen <= 0)
    {
        return;
    }

    m_pT = new T[iLen];
    if (NULL == m_pT)
    {
        return;
    }
}

template<typename T>
CCirQueue<T>::~CCirQueue()
{
    pthread_mutex_lock(&m_Mutex);
    if (NULL != m_pT)
    {
        delete []m_pT;
        m_pT = NULL;
    }

    m_iHead = 0;
    m_iTail = 0;
    m_iLen = 0;

    pthread_mutex_unlock(&m_Mutex);

    pthread_mutex_destroy(&m_Mutex);
}

template<typename T>
int CCirQueue<T>::cirQueueIn(const T & t)
{
    pthread_mutex_lock(&m_Mutex);
    if ((m_iTail + 1) % m_iLen == m_iHead)
    {
        pthread_mutex_unlock(&m_Mutex);
        return 0;
    }

    m_iTail = m_iTail  % m_iLen;
    m_pT[m_iTail++] = t;
    printf("m_iTail:%d\n", m_iTail);
    pthread_mutex_unlock(&m_Mutex);
    return 1;
}

template<typename T>
T & CCirQueue<T>::cirQueueGet(bool * pbOk)
{
    static T t;
    pthread_mutex_lock(&m_Mutex);
    if ((m_iTail + 1) % m_iLen == m_iHead)
    {
        if (NULL != pbOk)
        {
            *pbOk = false;
        }

        pthread_mutex_unlock(&m_Mutex);
        return t;
    }

    m_iTail = (m_iTail + 1) % m_iLen;

    if (NULL != pbOk)
    {
        *pbOk = true;
    }

    pthread_mutex_unlock(&m_Mutex);
    return m_pT[m_iTail];
}

template<typename T>
int CCirQueue<T>::cirQueueOut(T & t)
{
    pthread_mutex_lock(&m_Mutex);

    if (m_iTail == m_iHead)
    {
        pthread_mutex_unlock(&m_Mutex);
        return 0;
    }

    m_iHead = (m_iHead + 1) % m_iLen;
    t = m_pT[m_iHead];

    pthread_mutex_unlock(&m_Mutex);
    return 1;
}

template<typename T>
T & CCirQueue<T>::cirQueueOut(bool * pbOk)
{
    static T t;

    pthread_mutex_lock(&m_Mutex);

    if (m_iTail == m_iHead)
    {
        if (NULL != pbOk)
        {
            *pbOk = false;
        }

        pthread_mutex_unlock(&m_Mutex);
        return t;
    }

    m_iHead = m_iHead  % m_iLen;
    if (NULL != pbOk)
    {
        *pbOk = true;
    }
    printf("m_iHead:%d\n", m_iHead);


    pthread_mutex_unlock(&m_Mutex);
    return m_pT[m_iHead++];
}

template<typename T>
bool CCirQueue<T>::isEmpty()
{
    return (m_iTail == m_iHead);
}

template<typename T>
bool CCirQueue<T>::isFull()
{
    return ((m_iTail + 1) % m_iLen == m_iHead);
}

template<typename T>
unsigned int CCirQueue<T>::length()
{
    return (((m_iTail - m_iHead) + m_iLen) % m_iLen);
}

template<typename T>
void CCirQueue<T>::reset()
{
    pthread_mutex_lock(&m_Mutex);
    m_iHead = 0;
    m_iTail = 0;
    pthread_mutex_unlock(&m_Mutex);
}

#endif // CIRQUEUE_H
