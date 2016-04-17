bbetData = importdata('bb23.txt', ',');
bbhtData = importdata('bb21.txt', ',');
hcetData = importdata('hc23.txt', ',');

bbetIds = bbetData(:, 1);
bbhtIds = bbhtData(:, 1);
hcetIds = hcetData(:, 1);

validIds = intersect(bbetIds, intersect(bbhtIds, hcetIds));
bbetValidIndices = ismember(bbetIds, validIds);
bbhtValidIndices = ismember(bbhtIds, validIds);
hcetValidIndices = ismember(hcetIds, validIds);

bbetData = bbetData(bbetValidIndices, 2:end);
bbhtData = bbhtData(bbhtValidIndices, 2:end);
hcetData = hcetData(hcetValidIndices, 2:end);

numTransforms = length(validIds); 

bbHets = reshape(bbetData', 4, 4, []);
bbHhts= reshape(bbhtData', 4, 4, []);
hcHets = reshape(hcetData', 4, 4, []);


bbHhcs = zeros(4, 4, numTransforms);
for i = 1:numTransforms
    bbHhcs(:, :, i) = bbHets(:, :, i) / hcHets(:, :, i);
end

htHhcs = zeros(4, 4, numTransforms);
for i = 1:numTransforms
    htHhcs(:, :, i) = bbHhts(:, :, i) \ bbHhcs(:, :, i);
end

baselineT = htHhcs(1:3, 4, 1);

avgT = zeros(3,1);
avgR = zeros(3,3);
figure
hold on
skipped = 0;
for i = 1:numCells
    t = htHhcs(1:3, 4, i);
    R = htHhcs(1:3, 1:3, i);
    if norm(t - baselineT) > .03
        skipped = skipped + 1;
        continue
    end
    avgT = avgT + t;
    avgR = avgR + logm(R);
    mArrow3(t, t + R(:,1) * .1, 'color', 'red', 'stemWidth', 0.0002); 
end

avgT = avgT / (numTransforms - skipped)
avgR = expm(avgR / (numTransforms - skipped))
mArrow3(avgT, avgT + avgR(:,1) * .1, 'color', 'blue', 'stemWidth', 0.0002); 
