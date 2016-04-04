import sys
import numpy as np
import scipy as sp

hcValuesSerial = np.loadtxt('leftHandCamera.txt')
bbValuesSerial = np.loadtxt('bumblebee.txt')

if 2 * len(hcValuesSerial) != len(bbValuesSerial):
    sys.exit('Some tags were not detected. Please curate photos.')

numTransformations = len(hcValuesSerial) / 16

hcHets = np.zeros([4, 4, numTransformations])
bbHhts = np.zeros([4, 4, numTransformations])
bbHets = np.zeros([4, 4, numTransformations])

j = 0
for i in xrange(0, len(hcValuesSerial), 16):
    hcHets[:, :, j] = np.reshape(hcValuesSerial[i:i + 16], (4, 4)).T
    j += 1

j = 0
for i in xrange(0, len(bbValuesSerial), 32):
    bbHhts[:, :, j] = np.reshape(bbValuesSerial[i:i + 16], (4, 4)).T
    j += 1

j = 0
for i in xrange(16, len(bbValuesSerial), 32):
    bbHets[:, :, j] = np.reshape(bbValuesSerial[i:i + 16], (4, 4)).T
    j += 1

bbHhcs = np.zeros([4, 4, numTransformations])
for i in xrange(numTransformations):
    bbHhcs[:, :, i] = np.dot(bbHets[:, :, i], np.linalg.inv(hcHets[:, :, i]))

htHhcs = np.zeros([4, 4, numTransformations])
for i in xrange(numTransformations):
    htHhcs[:, :, i] = np.dot(np.linalg.inv(bbHhts[:, :, i]), bbHhcs[:, :, i])

avgT = np.zeros([3, 1])
avgR = np.zeros([3, 3])

for i in xrange(numTransformations):
    avgT += htHhcs[0:3, 3, i].reshape(3, 1)
    avgR += htHhcs[0:3, 0:3, i]

avgT = avgT / numTransformations
avgR = avgR / numTransformations

print avgT
print avgR
# for i = 1:numCells
#     t = htHhcArray{i}(1:3, 4);
#     R = htHhcArray{i}(1:3, 3);
#     mArrow3(t, t + R * .1, 'color', 'red', 'stemWidth', 0.0002);
# end
