#include <functional>
#include <boost/integer_traits.hpp>
#include <boost/python.hpp>
#include <sparsehash/dense_hash_map>

namespace bp = boost::python;
namespace gg = google;

template <class Key, class Val,
          Key Empty=boost::integer_traits<Key>::const_max,
          class HashFcn=std::tr1::hash<Key>,
          class EqualKey=std::equal_to<Key> >
class hashmap
    : public gg::dense_hash_map<Key, Val, HashFcn, EqualKey>
{
  private:
    typedef gg::dense_hash_map<Key, Val, HashFcn, EqualKey> super;

  public:
    hashmap(typename super::size_type n=0) : super(n) { this->set_empty_key(Empty); }
};

typedef hashmap<long, long> hashmap_int;

template<typename T>
struct map_adaptor
{
    typedef typename T::key_type key_type;
    typedef typename T::data_type data_type;
    typedef typename T::value_type value_type;
    typedef typename T::iterator iterator;

    static data_type& get(T& self, const key_type& k)
    {
        iterator it = self.find(k);
        if (it != self.end()) return it->second;
        PyErr_SetNone(PyExc_KeyError);
        throw bp::error_already_set();
    }

    static void set(T& self, const key_type& k, const data_type& d)
    {
        self.insert(value_type(k,d));
    }
};

BOOST_PYTHON_MODULE(pyhashmap)
{
    // make the bp automatic docstrings slightly less stupid.
    bp::docstring_options doc(true, false);

    bp::class_<hashmap_int>("hashmap", bp::no_init)
        .def(bp::init<bp::optional<hashmap_int::size_type> >())
        .def("__getitem__", &map_adaptor<hashmap_int>::get, bp::return_value_policy<bp::copy_non_const_reference>())
        .def("__setitem__", &map_adaptor<hashmap_int>::set, bp::with_custodian_and_ward<1,3>());
}
