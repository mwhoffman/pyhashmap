import pyhashmap


def test_pyhashmap():
    mymap = pyhashmap.hashmap(500000)
    mymap['asdf'] = 123
    assert mymap['asdf'] == 123

    mymap['2134'] = dict()
    mymap['2134']['asdf'] = 1
    a = mymap['2134']
    assert a['asdf'] == 1
