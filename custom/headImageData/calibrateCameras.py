import numpy as np
from tf import transformations

bbetData = np.loadtxt('bb98.txt', delimiter=',')
heetData = np.loadtxt('he98.txt', delimiter=',')

bbetIds = bbetData[:, 0]
heetIds = heetData[:, 0]

validIds = np.intersect1d(bbetIds, heetIds)
bbetValidIndices = np.in1d(bbetIds, validIds)
heetValidIndices = np.in1d(heetIds, validIds)

print bbetValidIndices
print heetValidIndices

bbetData = bbetData[bbetValidIndices, 1:]
heetData = heetData[heetValidIndices, 1:]

numTransforms = len(validIds)
bbHets = np.reshape(bbetData.T, (4, 4, -1), 3)
heHets = np.reshape(heetData.T, (4, 4, -1), 3)

# BB -> ET
# HE -> ET
# HE -> BB
# BB -> ET * inv(HE -> ET) * HE -> BB = I
# HE -> BB * inv(HE -> ET) * BB -> ET = I
# HE -> BB = inv(BB -> ET) * HE -> ET

heHbbs = np.zeros([4, 4, numTransforms])
for i in xrange(numTransforms):
    heHbbs[:, :, i] = np.dot(heHets[:, :, i], np.linalg.inv(bbHets[:, :, i]))

avgT = np.zeros([3, 1])
avgR = np.zeros([3, 3])

baseLiney = -.25

skipped = 0
for i in xrange(numTransforms):
    t = heHbbs[0:3, 3, i].reshape(3, 1)
    print t
    if abs(t[1] - baseLiney) > .03:
        skipped += 1
        continue
    avgT += t
    avgR += heHbbs[0:3, 0:3, i]

print skipped

avgT = avgT / (numTransforms - skipped)
avgR = avgR / (numTransforms - skipped)

u, s, v = np.linalg.svd(avgR)
avgR = np.dot(u, v)

paddedR = np.zeros((4, 4))
paddedR[3, 3] = 1
paddedR[0:3, 0:3] = avgR
quat = transformations.quaternion_from_matrix(paddedR)

print avgT
print quat

f = open('headCalibration.txt', 'w')
np.savetxt(f, avgT, newline='\n')
f.write('----------------------\n')
np.savetxt(f, quat, newline='\n')
