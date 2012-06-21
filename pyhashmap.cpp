#include <functional>
#include <limits>
#include <sparsehash/dense_hash_map>
#include <boost/python.hpp>

namespace bp = boost::python;
namespace gg = google;

template <
    class Key,
    class EmptyKey,
    class HashFcn=std::tr1::hash<Key>,
    class EqualKey=std::equal_to<Key> >
class pyhashmap
{
  private:
    typedef gg::dense_hash_map<Key, bp::object, HashFcn, EqualKey> map_type;
    map_type map;

  public:
    typedef typename map_type::size_type size_type;
    typedef typename map_type::key_type key_type;
    typedef typename map_type::value_type value_type;
    typedef typename map_type::iterator iterator;

    pyhashmap(size_type n=0) : map(n) { map.set_empty_key(EmptyKey()()); }

    bp::object getitem(const key_type& k)
    {
        iterator it = map.find(k);
        if (it != map.end())
            return it->second;
        PyErr_SetNone(PyExc_KeyError);
        throw bp::error_already_set();
    }

    void setitem(const key_type& k, const bp::object& v)
    {
        iterator it = map.find(k);
        if (it != map.end())
            it->second = v;
        else
            map.insert(value_type(k, v));
    }
};

template<typename T>
struct MaxKey {
    T operator()() const { return std::numeric_limits<T>::max(); }
};

struct EmptyString {
    std::string operator()() const { return "asdOFiaqjsdfBazxcvf"; }
};

typedef pyhashmap<long, MaxKey<long> > pyhashmap_int;
typedef pyhashmap<std::string, EmptyString> pyhashmap_str;

template<class PHM>
void create_wrapper(const char* classname)
{
    bp::class_<PHM>(classname, bp::no_init)
        .def(bp::init<bp::optional<typename PHM::size_type> >())
        .def("__getitem__", &PHM::getitem)
        .def("__setitem__", &PHM::setitem);
}

BOOST_PYTHON_MODULE(pyhashmap)
{
    // make the bp automatic docstrings slightly less stupid.
    bp::docstring_options doc(true, false);

    create_wrapper<pyhashmap_int>("hashmap_int");
    create_wrapper<pyhashmap_str>("hashmap_str");
}

