#include <functional>
#include <limits>
#include <sparsehash/dense_hash_map>
#include <boost/python.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <city.h>

namespace bp = boost::python;
namespace gg = google;

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

    typedef typename boost::transform_iterator<select_key, iterator> key_iterator;
    key_iterator begin_key() { return boost::make_transform_iterator(map.begin(), select_key()); }
    key_iterator end_key() { return boost::make_transform_iterator(map.end(), select_key()); }

    bp::object getitem(const key_type& k)
    {
        iterator it = map.find(k);
        if (it != map.end())
            return it->second;
        PyErr_SetNone(PyExc_KeyError);
        throw bp::error_already_set();
    }

    void delitem(const key_type& k)
    {
        iterator it = map.find(k);
        if (it != map.end())
            map.erase(it);
        else {
            PyErr_SetNone(PyExc_KeyError);
            throw bp::error_already_set();
        }
    }

    void setitem(const key_type& k, const bp::object& v)
    {
        iterator it = map.find(k);
        if (it != map.end())
            it->second = v;
        else
            map.insert(value_type(k, v));
    }

    size_type len() {
        return map.size();
    }

    bool contains(const key_type &k) {
        return map.find(k) != map.end();
    }
};

template<typename T>
struct IntKeys {
    static T Empty() { return std::numeric_limits<T>::max(); }
    static T Erase() { return std::numeric_limits<T>::max() - 1; }
};

struct StringKeys {
    static std::string Empty() { return "asdOFiaqjsdfBazxcvf"; }
    static std::string Erase() { return "8asflakjbl;sdfDSslf"; }
};

struct CityHashFcn {
    size_t operator()(const std::string &str) const {
        return CityHash64(str.c_str(), str.size());
    }
};

typedef pyhashmap<long, IntKeys<long> > pyhashmap_int;
typedef pyhashmap<std::string, StringKeys, CityHashFcn> pyhashmap_str;

template<class PHM>
void create_wrapper(const char* classname)
{
    bp::class_<PHM>(classname, bp::no_init)
        .def(bp::init<bp::optional<typename PHM::size_type> >())
        .def("__len__", &PHM::len)
        .def("__contains__", &PHM::contains)
        .def("__getitem__", &PHM::getitem)
        .def("__delitem__", &PHM::delitem)
        .def("__setitem__", &PHM::setitem)
        .def("__iter__", bp::range(&PHM::begin_key, &PHM::end_key))
        ;
}

BOOST_PYTHON_MODULE(pyhashmap)
{
    // make the bp automatic docstrings slightly less stupid.
    bp::docstring_options doc(true, false);

    create_wrapper<pyhashmap_int>("hashmap_int");
    create_wrapper<pyhashmap_str>("hashmap_str");
}

