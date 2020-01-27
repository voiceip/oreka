/*
 * Oreka -- A media capture and retrieval platform
 * 
 * Copyright (C) 2005, orecx LLC
 *
 * http://www.orecx.com
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License.
 * Please refer to http://www.gnu.org/copyleft/gpl.html
 *
 */

//This map data structure is not thread safe

#ifndef __ORKLRUMAP_H__
#define __ORKLRUMAP_H__
#include <iostream>
#include <list>
#include <map>
#include <thread>
#include <chrono>
#include <memory>
#include <mutex>

template<typename K>
using OrekaLruIterator = typename std::list<std::pair<K, uint64_t>>::iterator;


template<typename K, typename V>
using OrekaLruMapIterator = typename std::map<K, std::pair<V, OrekaLruIterator<K>>>::iterator;

template <class K, class V> class OrekaLruMap
{
public:
	OrekaLruMap(unsigned int size = 10000)
	{
		m_size = size;
	};
	//static MapIterator<K, V> Iterator;
	OrekaLruMapIterator<K, V> begin();
	OrekaLruMapIterator<K, V> end();
    OrekaLruIterator<K> lruBegin();
    OrekaLruIterator<K> lruEnd();
	OrekaLruMapIterator<K, V> Find(K key);
    OrekaLruMapIterator<K, V> GetOldestPair();
    OrekaLruMapIterator<K, V> GetNewestPair();
	void InsertPair(K key, V &val);
	bool Remove(K key);
	bool Remove(OrekaLruMapIterator<K, V> it);
	bool GetValue(K key, V& val);
    uint64_t GetEntryTs(K key);
    uint64_t GetEntryTs(OrekaLruMapIterator<K, V> it);
    uint64_t GetOldestTs();
    void RemoveOldest();
	void Clear();
    V &operator[](K key){
        return Find(key)->second.first;
    }

    unsigned int Count();
	unsigned int Size();
	unsigned int SetSize(unsigned int size);
private:
	unsigned int m_size;
	std::list<std::pair<K, uint64_t>> m_list;
	std::map<K, std::pair<V, OrekaLruIterator<K>>> m_map;
};

template <class K, class V> OrekaLruMapIterator<K, V> OrekaLruMap<K, V>::begin()
{
	return m_map.begin();
}

template <class K, class V> OrekaLruMapIterator<K, V> OrekaLruMap<K, V>::end()
{
	return m_map.end();
}

template <class K, class V> OrekaLruIterator<K> OrekaLruMap<K, V>::lruBegin()
{
	return m_list.begin();
}

template <class K, class V> OrekaLruIterator<K> OrekaLruMap<K, V>::lruEnd()
{
	return m_list.end();
}

template <class K, class V> OrekaLruMapIterator<K, V> OrekaLruMap<K, V>::GetOldestPair()
{
    if(m_list.size() > 0)
    {
        return m_map.find(m_list.front().first);
    }
    else
    {
        return m_map.end();
    }
	
}

template <class K, class V> OrekaLruMapIterator<K, V> OrekaLruMap<K, V>::GetNewestPair()
{
    if(m_list.size() > 0)
    {
        return m_map.find(m_list.back().first);
    }
    else
    {
        return m_map.end();
    }
	
}

template <class K, class V> void OrekaLruMap<K, V>::InsertPair(K key, V &val)
{
	if(m_map.size() >= m_size)
	{
		K removeKey = m_list.front().first;
		m_map.erase(removeKey);
		m_list.pop_front();
	}

	//check if key exist
	OrekaLruMapIterator<K, V> it = m_map.find(key);
	if(it != m_map.end()){
		OrekaLruIterator<K> it2 = it->second.second;
		m_list.erase(it2);
		m_map.erase(it);
	}

	m_list.push_back(std::make_pair(key, time(NULL)));
	auto it3 = m_list.end(); --it3;
	m_map.insert(std::make_pair(key, std::make_pair(val, it3)));
}

template <class K, class V> OrekaLruMapIterator<K, V> OrekaLruMap<K, V>::Find(K key)
{
	return m_map.find(key);
}

template <class K, class V> bool OrekaLruMap<K, V>::GetValue(K key, V& val)
{
    bool ret = false;
	OrekaLruMapIterator<K, V> it = Find(key);
	if(it != m_map.end())
	{
		val = it->second.first;
        ret = true;
	}
	else
    {
        ret = false;
    }
	return ret;
}

template <class K, class V> uint64_t OrekaLruMap<K, V>::GetEntryTs(K key)
{
    uint64_t ts = -1;
    OrekaLruMapIterator<K, V> it = Find(key);
	if(it != m_map.end())
	{
        ts = (*(it->second.second)).second;
	}

	return ts;
}

template <class K, class V> uint64_t OrekaLruMap<K, V>::GetEntryTs(OrekaLruMapIterator<K, V> it)
{
    uint64_t ts = -1;
	if(it != m_map.end())
	{
        ts = (*(it->second.second)).second;
	}

	return ts;
}

template <class K, class V> uint64_t OrekaLruMap<K, V>::GetOldestTs()
{
    if(m_list.size() > 0)
    {
        return m_list.front().second;
    }
    else
    {
        return -1;
    }    
}

template <class K, class V> void OrekaLruMap<K, V>::RemoveOldest()
{
    if(m_list.size() > 0)
    {
        K key = m_list.front().first;
        m_map.erase(key);
        m_list.pop_front();
    }
}

template <class K, class V> bool OrekaLruMap<K, V>::Remove(K key)
{
	bool ret = false;
	OrekaLruMapIterator<K, V> it = m_map.find(key);
	if(it != m_map.end()){
		OrekaLruIterator<K> it2 = it->second.second;
		m_list.erase(it2);
		m_map.erase(key);
		ret = true;
	}
	return ret;
}

template <class K, class V> bool OrekaLruMap<K, V>::Remove(OrekaLruMapIterator<K, V> it)
{
	return Remove(it->first);
}

template <class K, class V> void OrekaLruMap<K, V>::Clear()
{
	m_map.clear();
	m_list.clear();
}

template <class K, class V> unsigned int OrekaLruMap<K, V>::Count()
{
	return m_map.size();
}

template <class K, class V> unsigned int OrekaLruMap<K, V>::Size()
{
	return m_size;
}

template <class K, class V> unsigned int OrekaLruMap<K, V>::SetSize(unsigned int size)
{
	if(size >= m_size)
    {
        m_size = size;
    }
    
    return m_size;
}

#endif 

 
 
