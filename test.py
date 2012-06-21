import pyhashmap

def test_pyhashmap():
    mymap = pyhashmap.hashmap_int(500000)
    a = dict()
    mymap[123] = a
    mymap[123]['asdf'] = 1
    assert a['asdf'] == 1

    mymap = pyhashmap.hashmap_str(500000)
    mymap['asdf'] = 123
    assert mymap['asdf'] == 123

    mymap['2134'] = dict()
    mymap['2134']['asdf'] = 1
    a = mymap['2134']
    assert a['asdf'] == 1

