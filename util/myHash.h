/****************************************************************************
  FileName     [ myHash.h ]
  PackageName  [ util ]
  Synopsis     [ Define Hash and Cache ADT ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2009-2010 LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef MY_HASH_H
#define MY_HASH_H

#include <vector>

using namespace std;

//--------------------
// Define Hash classes
//--------------------
// To use Hash ADT, you should define your own HashKey class.
// It should at least overload the "()" and "==" operators.
//
// class HashKey
// {
// public:
//    HashKey() {}
// 
//    size_t operator() () const { return 0; }
// 
//    bool operator == (const HashKey& k) { return true; }
// 
// private:
// };
//
template <class HashKey, class HashData>
class Hash
{
typedef pair<HashKey, HashData> HashNode;

public:
   Hash(size_t b = 0) : _numBuckets(0), _buckets(NULL) { init(b); }
   ~Hash() { reset(); }

   // TODO: implement the Hash<HashKey, HashData>::iterator
   // o An iterator should be able to go through all the valid HashNodes
   //   in the Hash
   // o Functions to be implemented:
   //   - constructor(s), destructor
   //   - operator '*': return the HashNode
   //   - ++/--iterator, iterator++/--
   //   - operators '=', '==', !="
   //
   // (_bId, _bnId) range from (0, 0) to (_numBuckets, 0)
   //
   class iterator
   {
      friend class Hash<HashKey, HashData>;

   public:
      iterator(Hash<HashKey, HashData> &h, int bucket, int index):
         _hash(h), _currBucket(bucket), _currIndex(index) {}
      iterator(const iterator &copy):
         _hash(copy._hash), _currBucket(copy._currBucket), 
         _currIndex(copy._currIndex) {}
      ~iterator() {}

      HashNode &operator*() const {
         return _hash[_currBucket][_currIndex];
      }

      iterator &operator++() {
         move_next();
         return *this;
      }
      iterator operator++(int) {
         iterator tmp(*this);
         ++*this;
         return tmp;
      }
      iterator &operator--() {
         move_prev();
         return *this;
      }
      iterator &operator--(int) {
         iterator tmp(*this);
         --*this;
         return tmp;
      }

      iterator &operator=(const iterator &rhs) {
         _hash = rhs._hash;
         _currBucket = rhs._currBucket;
         _currIndex = rhs._currIndex;
      }

      bool operator==(const iterator &rhs) const {
         return (_hash == rhs._hash && _currBucket == rhs._currBucket &&
               _currIndex == rhs._currIndex);
      }

      bool operator!=(const iterator &rhs) const {
         return !(*this == rhs);
      }

   private:
      Hash<HashKey, HashData> &_hash;
      int _currBucket, _currIndex;

      void move_first() {
         _currIndex = 0;
         size_t num = _hash.numBuckets();
         for(_currBucket = 0; _currBucket < num; ++_currBucket)
            if(_hash._buckets[_currBucket].size() > 0) break;
      }

      void move_next() {
         size_t num = _hash.numBuckets();
         ++_currIndex;
         while(_currBucket < num && _currIndex >= num) {
            _currIndex = 0;
            ++_currBucket;
         }
      }

      void move_prev() {
         --_currIndex;
         while(_currBucket >= 0 && _currIndex < 0) {
            --_currBucket;
            _currIndex = _hash._buckets[_currBucket].size();
         }
      }
   };

   // TODO: implement these functions
   //
   // Point to the first valid data
   iterator begin() const {
      iterator tmp(*this, 0, 0);
      tmp.move_first();
      return tmp;
   }
   // Pass the end
   iterator end() const {
      return iterator(*this, _numBuckets, 0);
   }
   // return true if no valid data
   bool empty() const {
      for(int i = 0; i < _numBuckets; ++i)
         if(_buckets[i].size() > 0) return false;
      return true;
   }
   // number of valid data
   size_t size() const {
      size_t s = 0;
      for(int i = 0; i < _numBuckets; ++i)
         s += _buckets[i].size();
      return s;
   }
   size_t numBuckets() const { return _numBuckets; }

   vector<HashNode>& operator [] (size_t i) { return _buckets[i]; }
   const vector<HashNode>& operator [](size_t i) const { return _buckets[i]; }

   void init(size_t b) {
      reset();
      _numBuckets = b;
      _buckets = new vector<HashNode>[b];
   }
   void reset() {
      if(!_buckets) return;
      delete[] _buckets;
      _buckets = NULL;
   }

   // check if k is in the hash...
   // if yes, update n and return true;
   // else return false;
   bool check(const HashKey& k, HashData& n) {
      vector<HashNode> &b = _buckets[bucketNum(k)];
      for(int i = b.size()-1; i >= 0; --i) {
         HashNode &node = b.at(i);
         if(node.first == k) {
            n = node.second;
            return true;
         }
      }
      return false;
   }

   // return true if inserted successfully (i.e. k is not in the hash)
   // return false is k is already in the hash ==> will not insert
   bool insert(const HashKey& k, const HashData& d) {
      HashData tmp;
      if(check(k, tmp)) return false;
      forceInsert(k, d);
      return true;
   }

   // return true if inserted successfully (i.e. k is not in the hash)
   // return false is k is already in the hash ==> still do the insertion
   bool replaceInsert(const HashKey& k, const HashData& d) {
      vector<HashNode> *b = _buckets[bucketNum(k)];
      for(int i = b->size()-1; i >= 0; --i) {
         HashNode &node = b->at(i);
         if(node.first == k) {
            node.second = d;
            return false;
         }
      }
      forceInsert(k, d);
      return true;
   }


   // Need to be sure that k is not in the hash
   void forceInsert(const HashKey& k, const HashData& d) {
      _buckets[bucketNum(k)].push_back(HashNode(k, d));
   }

private:
   // Do not add any extra data member
   size_t                   _numBuckets;
   vector<HashNode>*        _buckets;

   size_t bucketNum(const HashKey& k) const {
      return (k() % _numBuckets);
   }
};


//---------------------
// Define Cache classes
//---------------------
// To use Cache ADT, you should define your own HashKey class.
// It should at least overload the "()" and "==" operators.
//
// class CacheKey
// {
// public:
//    CacheKey() {}
//    
//    size_t operator() () const { return 0; }
//   
//    bool operator == (const CacheKey&) const { return true; }
//       
// private:
// }; 
// 
template <class CacheKey, class CacheData>
class Cache
{
typedef pair<CacheKey, CacheData> CacheNode;

public:
   Cache(size_t s = 1) : _cache(NULL) { init(s); }
   ~Cache() {
      if(_cache) delete[] _cache;
   }

   // NO NEED to implement Cache::iterator class

   // TODO: implement these functions
   //
   // Initialize _cache with size s
   void init(size_t s) {
      _size = s;
      reset();
   }
   void reset() {
      if(_cache) delete[] _cache;
      _cache = new CacheNode[_size];
   }

   size_t size() const { return _size; }

   CacheNode& operator [] (size_t i) { return _cache[i]; }
   const CacheNode& operator [](size_t i) const { return _cache[i]; }

   // return false if cache miss
   bool read(const CacheKey& k, CacheData& d) const {
      if(!k.valid()) return false;
      d = _cache[getIdx(k)].second;
   }
   // If k is already in the Cache, overwrite the CacheData
   void write(const CacheKey& k, const CacheData& d) {
      _cache[getIdx(k)] = d;
   }

private:
   // Do not add any extra data member
   size_t         _size;
   CacheNode*     _cache;

   int getIdx(const CacheKey &k) { return k()%_size; }
};


#endif // MY_HASH_H
