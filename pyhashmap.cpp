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
    struct select_key
        : std::unary_function<const value_type&, const key_type&>
    {
        const key_type& operator()(const value_type& p) const { return p.first; }
    };  

  public:
    pyhashmap(size_type n=0) : map(n) {
        map.set_empty_key(SpecialKeys::Empty());
        map.set_deleted_key(SpecialKeys::Erase());
    }

    // iterator which only looks at keys.
    typedef typename boost::transform_iterator<select_key, iterator> key_iterator;

    // simple iterator access, length, and containment.
    inline key_iterator begin() { return boost::make_transform_iterator(map.begin(), select_key()); }
    inline key_iterator end() { return boost::make_transform_iterator(map.end(), select_key()); }
    inline size_type len() { return map.size(); }
    inline bool contains(const key_type &k) { return map.find(k) != map.end(); }

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
// Specific functors to use for std::string's. StringKeys defines which keys to
// use for marking empty and erased components within the hash implementation.
// For strings we can also use the CityHash64 hash functions. These aren't
// really cryptographically secure, but they're super fast and give good
// coverage of the hashed space. The 64-bit version plays nicely with our 64-bit
// machine.

struct StringKeys {
    static std::string Empty() { return "asdOFiaqjsdfBazxcvf"; }
    static std::string Erase() { return "8asflakjbl;sdfDSslf"; }
};

// For strings the CityHash method is really fast and it gives better uniform
// coverage of the hashed space.
struct CityHashFcn {
    size_t operator()(const std::string &str) const {
        return CityHash64(str.c_str(), str.size());
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
        .def("__iter__", bp::range(&C::begin, &C::end));
}

BOOST_PYTHON_MODULE(pyhashmap)
{
    // make the bp automatic docstrings slightly less stupid.
    bp::docstring_options doc(true, false);

    create_map_wrapper<pyhashmap<std::string, StringKeys, CityHashFcn> >("hashmap");

    create_container_wrapper<pysparsetable>("sparsetable")
        .def("itervalues", bp::range(&pysparsetable::begin_values, &pysparsetable::end_values));
}

