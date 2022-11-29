import unittest,sys
from ctypes import *
sys.path.append('/src/python')
from animations import RandomWalkAnimation

class cRandomWalkAnimation:
    @classmethod
    def Init(cls,path_to_library):
        cls.lib = CDLL(path_to_library)
    
    def __init__(self):
        c_int_p   = POINTER(c_int)
        c_float_p = POINTER(c_float)
        find_closest_i = cRandomWalkAnimation.lib.find_closest_i
        find_closest_i.restype = c_int
        find_closest_i.argtypes = [c_int_p, c_int, c_int_p]
        self.find_closest_i = find_closest_i

        find_closest_f = cRandomWalkAnimation.lib.find_closest_f
        find_closest_f.restype = c_int
        find_closest_f.argtypes = [c_float_p, c_int, c_int_p]
        self.find_closest_f = find_closest_f

    def find_closest(self,vect,value):
        N = len(vect)
        if type(value) == int:
            Arr_n_int = c_int * N
            vector = Arr_n_int(*vect)
            Arr_2_int = c_int * 2
            res = Arr_2_int()
            n = self.find_closest_i(pointer(vector),value,pointer(res))
        elif type(value) == float:
            pass
        raise NotImplemented

class RandomWalkAnimation_TestCase(unittest.TestCase):
    def setUp(self):
        cRandomWalkAnimation.Init('./build/librandom-walk-animation.so')
    def test_001(self):
        #neighbours are indices!
        anim = cRandomWalkAnimation()
        anim.find_closest([1,2,3],1)
        c = RandomWalkAnimation.findClosest( [1,2,3],1)
        self.assertEqual(c,[0])

        c = RandomWalkAnimation.findClosest( [1,2,3],2)
        self.assertEqual(c,[1])

        c = RandomWalkAnimation.findClosest( [1,2,3],3)
        self.assertEqual(c,[2])

        c = RandomWalkAnimation.findClosest( [1,2,3],1.5)
        self.assertEqual(c,[0,1])

        c = RandomWalkAnimation.findClosest( [1,2,3],2.5)
        self.assertEqual(c,[1,2])

        c = RandomWalkAnimation.findClosest( [1,2,3],3.5)
        self.assertEqual(c,[2])

unittest.main(argv=[__file__])
