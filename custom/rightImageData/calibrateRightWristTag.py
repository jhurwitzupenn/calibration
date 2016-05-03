import numpy as np
from tf import transformations

tagID = 124
bbetData = np.loadtxt('bb23.txt', delimiter=',')
bbhtData = np.loadtxt('bb{0}.txt'.format(tagID), delimiter=',')
hcetData = np.loadtxt('hc23.txt', delimiter=',')
rotations = np.loadtxt('rotations.txt', delimiter=',')
translations = np.loadtxt('translations.txt', delimiter=',')

bbetIds = bbetData[:, 0]
bbhtIds = bbhtData[:, 0]
hcetIds = hcetData[:, 0]
rotationIds = rotations[:, 0]
translationIds = translations[:, 0]

validDataIds = np.intersect1d(bbetIds, np.intersect1d(bbhtIds, hcetIds))
validTransformIds = np.intersect1d(rotationIds, translationIds)
validIds = np.intersect1d(validDataIds, validTransformIds)
bbetValidIndices = np.in1d(bbetIds, validIds)
bbhtValidIndices = np.in1d(bbhtIds, validIds)
hcetValidIndices = np.in1d(hcetIds, validIds)
rotationValidIndices = np.in1d(rotationIds, validIds)
translationValidIndices = np.in1d(translationIds, validIds)

bbetData = bbetData[bbetValidIndices, 1:]
bbhtData = bbhtData[bbhtValidIndices, 1:]
hcetData = hcetData[hcetValidIndices, 1:]
rotations = rotations[rotationValidIndices, 1:]
translations = translations[translationValidIndices, 1:]

numTransforms = len(validIds)
bbHets = np.reshape(bbetData.T, (4, 4, -1), 3)
bbHhts = np.reshape(bbhtData.T, (4, 4, -1), 3)
hcHets = np.reshape(hcetData.T, (4, 4, -1), 3)
faHhcs = np.zeros([4, 4, numTransforms])

for i in xrange(numTransforms):
    rotation = transformations.quaternion_matrix(rotations[i, :])
    translation = translations[i, :]
    rotation[0:3, 3] = translation
    faHhcs[:, :, i] = rotation

htHfas = np.zeros([4, 4, numTransforms])
for i in xrange(numTransforms):
    htHfas[:, :, i] = np.dot(np.linalg.inv(bbHhts[:, :, i]),
                             np.dot(bbHets[:, :, i],
                                    np.dot(np.linalg.inv(hcHets[:, :, i]),
                                           np.linalg.inv(faHhcs[:, :, i]))))

avgT = np.zeros([3, 1])
avgR = np.zeros([3, 3])

for i in xrange(numTransforms):
    t = htHfas[0:3, 3, i].reshape(3, 1)
    avgT += t
    avgR += htHfas[0:3, 0:3, i]

avgT = avgT / (numTransforms)
avgR = avgR / (numTransforms)

u, s, v = np.linalg.svd(avgR)
avgR = np.dot(u, v)

paddedR = np.zeros((4, 4))
paddedR[3, 3] = 1
paddedR[0:3, 0:3] = avgR
quat = transformations.quaternion_from_matrix(paddedR)

print avgT
print quat

f = open('rightWristCalibration{0}.txt'.format(tagID), 'w')
np.savetxt(f, avgT, newline='\n')
f.write('----------------------\n')
np.savetxt(f, quat, newline='\n')
