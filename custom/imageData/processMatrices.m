hcValues = importdata('leftHandCamera.txt', '\n');
bbValues = importdata('bumblebee.txt', '\n');


numCells = length(hcValues) / 16; 

hcHetArray = cell(numCells,1);
bbHhtArray = cell(numCells,1);
bbHetArray = cell(numCells,1);

j = 1;
for i = 1:16:length(hcValues)
    hcHetArray(j) = num2cell(reshape(hcValues(i:i+15), [4, 4]), [1 2]);
    j = j + 1;
end

j = 1;
for i = 1:32:length(bbValues)
    bbHhtArray(j) = num2cell(reshape(bbValues(i:i+15), [4, 4]), [1 2]);
    j = j + 1;
end

j = 1;
for i = 17:32:length(bbValues)
    bbHetArray(j) = num2cell(reshape(bbValues(i:i+15), [4, 4]), [1 2]);
    j = j + 1;
end

bbHhcArray = cell(numCells,1);
for i = 1:numCells
    bbHhcArray(i) = num2cell(bbHetArray{i} / hcHetArray{i}, [1 2]);
end

htHhcArray = cell(numCells,1);
for i = 1:numCells
    htHhcArray(i) = num2cell(bbHhtArray{i} \ bbHhcArray{i} , [1 2]);
end

figure
hold on
for i = 1:numCells
    t = htHhcArray{i}(1:3, 4);
    R = htHhcArray{i}(1:3, 3);
    mArrow3(t, t + R * .1, 'color', 'red', 'stemWidth', 0.0002); 
end