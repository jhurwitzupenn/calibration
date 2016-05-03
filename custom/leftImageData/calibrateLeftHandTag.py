import numpy as np
from tf import transformations

bbetData = np.loadtxt('bb23.txt', delimiter=',')
bbhtData = np.loadtxt('bb21.txt', delimiter=',')
hcetData = np.loadtxt('hc23.txt', delimiter=',')

bbetIds = bbetData[:, 0]
bbhtIds = bbhtData[:, 0]
hcetIds = hcetData[:, 0]

validIds = np.intersect1d(bbetIds, np.intersect1d(bbhtIds, hcetIds))
bbetValidIndices = np.in1d(bbetIds, validIds)
bbhtValidIndices = np.in1d(bbhtIds, validIds)
hcetValidIndices = np.in1d(hcetIds, validIds)

bbetData = bbetData[bbetValidIndices, 1:]
bbhtData = bbhtData[bbhtValidIndices, 1:]
hcetData = hcetData[hcetValidIndices, 1:]

numTransforms = len(validIds)
bbHets = np.reshape(bbetData.T, (4, 4, -1), 3)
bbHhts = np.reshape(bbhtData.T, (4, 4, -1), 3)
hcHets = np.reshape(hcetData.T, (4, 4, -1), 3)
rlfHhc = np.zeros([4, 4, numTransforms])

bbHhcs = np.zeros([4, 4, numTransforms])
for i in xrange(numTransforms):
    bbHhcs[:, :, i] = np.dot(bbHets[:, :, i], np.linalg.inv(hcHets[:, :, i]))

htHhcs = np.zeros([4, 4, numTransforms])
for i in xrange(numTransforms):
    htHhcs[:, :, i] = np.dot(np.linalg.inv(bbHhts[:, :, i]), bbHhcs[:, :, i])

avgT = np.zeros([3, 1])
avgR = np.zeros([3, 3])

for i in xrange(numTransforms):
    t = htHhcs[0:3, 3, i].reshape(3, 1)
    avgT += htHhcs[0:3, 3, i].reshape(3, 1)
    avgR += htHhcs[0:3, 0:3, i]

avgT = avgT / numTransforms
avgR = avgR / numTransforms

u, s, v = np.linalg.svd(avgR)
avgR = np.dot(u, v)

paddedR = np.zeros((4, 4))
paddedR[3, 3] = 1
paddedR[0:3, 0:3] = avgR
quat = transformations.quaternion_from_matrix(paddedR)

print avgT
print quat

f = open('leftHandCalibration.txt', 'w')
np.savetxt(f, avgT, newline='\n')
f.write('----------------------\n')
np.savetxt(f, quat, newline='\n')
