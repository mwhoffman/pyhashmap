#include <functional>
#include <string>
#include <sparsehash/dense_hash_map>
#include <sparsehash/sparsetable>
#include <boost/python.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <city.h>

namespace bp = boost::python;
namespace gg = google;

//==============================================================================
// pypair
// A little helper type which defines functors to select the first and second
// elements from a pair. So given a type T which has the same semantics as the
// std::pair type, pypair<T>::first is a functor to select the first
// element (and vice-versa for ::second).

template<typename T>
struct pypair
{
    typedef T pair_type;
    typedef typename pair_type::first_type first_type;
    typedef typename pair_type::second_type second_type;

    // functor to select the first item.
    struct first: std::unary_function<const pair_type&, const first_type&>
    {
        const first_type& operator()(const pair_type& p) const {
            return p.first;
        }
    };

    // functor to select the second item.
    struct second : std::unary_function<const pair_type&, const second_type&>
    {
        const second_type& operator()(const pair_type& p) const {
            return p.second;
        }
    };

    // functor to convert the pair into a python tuple.
    struct to_tuple : std::unary_function<const pair_type&, bp::tuple>
    {
        bp::tuple operator()(const pair_type& p) const {
            return bp::make_tuple(p.first, p.second);
        }
    };
};

//==============================================================================
// pyhashmap
// Hashmap class which wraps google's dense_hash_map implementation, meaning its
// really fast. Without any arguments this uses the standard hash function for
// the given Key type, but for strings we should probably use something like
// CityHash.

template <
    class Key,
    class SpecialKeys,
    class HashFcn=std::tr1::hash<Key>,
    class EqualFcn=std::equal_to<Key> >
class pyhashmap
{
  private:
    typedef gg::dense_hash_map<Key, bp::object, HashFcn, EqualFcn> map_type;
    map_type map;

  public:
    typedef typename map_type::size_type size_type;
    typedef typename map_type::key_type key_type;
    typedef typename map_type::data_type data_type;
    typedef typename map_type::value_type value_type;
    typedef typename map_type::iterator iterator;

  private:
    typedef typename pypair<value_type>::first select_key;
    typedef typename pypair<value_type>::second select_val;
    typedef typename pypair<value_type>::to_tuple to_tuple;

  public:
    pyhashmap(size_type n=0) : map(n) {
        map.set_empty_key(SpecialKeys::Empty());
        map.set_deleted_key(SpecialKeys::Erase());
    }

    // transform iterators to get keys, data, etc.
    typedef typename boost::transform_iterator<select_key, iterator> key_iterator;
    typedef typename boost::transform_iterator<select_val, iterator> val_iterator;
    typedef typename boost::transform_iterator<to_tuple, iterator> item_iterator;

    // iterator over the keys.
    inline key_iterator begin() {
        return boost::make_transform_iterator(map.begin(), select_key());
    }

    inline key_iterator end() {
        return boost::make_transform_iterator(map.end(), select_key());
    }

    // iterator over the data.
    inline val_iterator begin_values() {
        return boost::make_transform_iterator(map.begin(), select_val());
    }
    
    inline val_iterator end_values() {
        return boost::make_transform_iterator(map.end(), select_val());
    }

    // iterator over the data.
    inline item_iterator begin_item() {
        return boost::make_transform_iterator(map.begin(), to_tuple());
    }
    
    inline item_iterator end_item() {
        return boost::make_transform_iterator(map.end(), to_tuple());
    }

    inline size_type len() const {
        return map.size();
    }
    
    inline bool contains(const key_type &k) const {
        return map.find(k) != map.end();
    }

    bp::object getitem(const key_type& k) {
        iterator it = map.find(k);
        if (it != map.end())
            return it->second;
        PyErr_SetNone(PyExc_KeyError);
        throw bp::error_already_set();
    }

    void delitem(const key_type& k) {
        iterator it = map.find(k);
        if (it != map.end())
            map.erase(it);
        else {
            PyErr_SetNone(PyExc_KeyError);
            throw bp::error_already_set();
        }
    }

    void setitem(const key_type& k, const bp::object& v) {
        iterator it = map.find(k);
        if (it != map.end())
            it->second = v;
        else
            map.insert(value_type(k, v));
    }
};

//==============================================================================
// pysparsetable
// Wrapper around sparsetable, which is essentially an array, but has better
// memory properties and acts like a "hash" map, but for ints.

// NOTE: currently I'm not dealing with any possible negative indices. So now
// the question is, should I? And I think I shouldn't, since this object is
// really acting more like a dict than an array.

class pysparsetable
{
  private:
    typedef gg::sparsetable<bp::object> table_type;
    table_type table;

  public:
    typedef table_type::size_type size_type;
    typedef table_type::value_type value_type;
    typedef table_type::iterator iterator;
    typedef table_type::nonempty_iterator nonempty_iterator;

    pysparsetable(size_type n) : table(n) {}

    // iterators over the values.
    inline nonempty_iterator begin_values() { return table.nonempty_begin(); }
    inline nonempty_iterator end_values() { return table.nonempty_end(); }

    inline size_type len() { return table.num_nonempty(); }
    inline bool contains(size_type i) { return table.test(i); }

    bp::object getitem(size_type i) {
        if (table.test(i))
            return table[i];
        PyErr_SetNone(PyExc_KeyError);
        throw bp::error_already_set();
    }

    void delitem(size_type i) {
        if (table.test(i))
            table.erase(i);
        else {
            PyErr_SetNone(PyExc_KeyError);
            throw bp::error_already_set();
        }
    }

    void setitem(size_type i, const bp::object& v) {
        // we don't have to worry about checking since we'll overwrite it
        // whatever is there anyways, and the destructor of the previously
        // existing object will take care of decrementing the refcount.
        table[i] = v;
    }
};

//==============================================================================
// Specific functors to use for different types.

// Keys to use for marking empty and erased components on strings. These were
// just chosen randomly, but I should probably make them something more
// reasonable.
struct StringKeys {
    static std::string Empty() { return "asdOFiaqjsdfBazxcvf"; }
    static std::string Erase() { return "8asflakjbl;sdfDSslf"; }
};

// For strings we can also use the CityHash64 hash functions. These aren't
// really cryptographically secure, but they're super fast and give good
// coverage of the hashed space. The 64-bit version plays nicely with our 64-bit
// machine.
struct CityHashFcn {
    inline size_t operator()(const std::string &str) const {
        return CityHash64(str.c_str(), str.size());
    }
};

// For integer keys just use the maximum two values.
template<typename T>
struct IntegerKeys {
    static T Empty() { return std::numeric_limits<T>::max(); }
    static T Erase() { return std::numeric_limits<T>::max()-1; }
};

// This is just a hack to apply the hashing methods to integers. Here I don't
// really want to do any hashing, so we can just use the identity integer
// itself. For the moment this is easier than delving into the dense table
// implementation.
template<typename T>
struct IdentityHash {
    inline size_t operator()(const T &myint) const {
        return myint;
    }
};

//==============================================================================
// create the python wrappers.

template<class C>
bp::class_<C> create_container_wrapper(const char* classname)
{
    return bp::class_<C>(classname, bp::no_init)
        .def(bp::init<typename C::size_type >())
        .def("__len__", &C::len)
        .def("__contains__", &C::contains)
        .def("__getitem__", &C::getitem)
        .def("__delitem__", &C::delitem)
        .def("__setitem__", &C::setitem);
}

template<class C>
bp::class_<C> create_map_wrapper(const char* classname)
{
    return create_container_wrapper<C>(classname)
        .def("__iter__", bp::range(&C::begin, &C::end))
        .def("itervalues", bp::range(&C::begin_values, &C::end_values))
        .def("iteritems", bp::range(&C::begin_item, &C::end_item));
}

BOOST_PYTHON_MODULE(pyhashmap)
{
    // make the bp automatic docstrings slightly less stupid.
    bp::docstring_options doc(true, false);

    create_map_wrapper<pyhashmap<std::string, StringKeys, CityHashFcn> >("hashmap");
    create_map_wrapper<pyhashmap<long, IntegerKeys<long>, IdentityHash<long> > >("table");

    create_container_wrapper<pysparsetable>("sparsetable")
        .def("itervalues", bp::range(&pysparsetable::begin_values, &pysparsetable::end_values));
}

