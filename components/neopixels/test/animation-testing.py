import unittest,sys,ctypes
sys.path.append('/src/python')
from animations import RandomWalkAnimation

class RandomWalkAnimation_TestCase(unittest.TestCase):
    def test_001(self):
        #neighbours are indices!
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
