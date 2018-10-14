clear all
clc

samplesCur = csvread('D:/data/out/samplesCur.csv');
samplesRef = csvread('D:/data/out/samplesRef.csv');

wCur = [0 0 0];
wRef = [0 0 0];

matA = zeros(3, 3);
matB = zeros(3, 3);
for i = 1:172
    for j=1:224        
        idx = (j - 1)*4 + 1;
        
        curDepth = samplesCur(i, idx+3);
        refDepth = samplesRef(i, idx+3);
        
        if(curDepth == 0)
            continue;
        elseif(refDepth == 0)
            continue;
        elseif(abs(abs(refDepth) - abs(curDepth)) >= 0.01)
            continue;
        end
        
        curSample = samplesCur(i, idx:idx+2);
        refSample = samplesRef(i, idx:idx+2);
        
        curHSV = rgb2hsv(curSample);
        refHSV = rgb2hsv(refSample);
        
        difH = curHSV(1) - refHSV(1);
		difS = curHSV(2) - refHSV(2);
		dist = sqrt(difH*difH + difS*difS);

		wAt = (1 - min(1.0, dist))^2.0;
		wCt = (1 - min(1.0, dist))^8.0;        
        
        wCur = wCur + wAt*curSample;
		wRef = wRef + wAt*refSample;
        
        wCurSample = curSample*wCt;
        
        matA(1, 1) = matA(1, 1) + wCurSample(1) * curSample(1);
		matA(2, 2) = matA(2, 2) + wCurSample(2) * curSample(2);
		matA(3, 3) = matA(3, 3) + wCurSample(3) * curSample(3);
		matA(1, 2) = matA(1, 2) + wCurSample(1) * curSample(2);
		matA(1, 3) = matA(1, 3) + wCurSample(1) * curSample(3);
		matA(2, 3) = matA(2, 3) + wCurSample(2) * curSample(3);

		matB(1, 1) = matB(1, 1) + wCurSample(1) * refSample(1);
		matB(2, 2) = matB(2, 2) + wCurSample(2) * refSample(2);
		matB(3, 3) = matB(3, 3) + wCurSample(3) * refSample(3);
		matB(1, 2) = matB(1, 2) + wCurSample(1) * refSample(2);
		matB(1, 3) = matB(1, 3) + wCurSample(1) * refSample(3);
		matB(2, 1) = matB(2, 1) + wCurSample(2) * refSample(1);
		matB(2, 3) = matB(2, 3) + wCurSample(2) * refSample(3);
		matB(3, 1) = matB(3, 1) + wCurSample(3) * refSample(1);
		matB(3, 2) = matB(3, 2) + wCurSample(3) * refSample(2);
    end
end

matA(2, 1) = matA(1, 2);
matA(3, 1) = matA(1, 3);
matA(3, 2) = matA(2, 3);

s = wRef./wCur;

gamma = 0.05;

matA(1, 1) = matA(1, 1) + gamma;
matA(2, 2) = matA(2, 2) + gamma;
matA(3, 3) = matA(3, 3) + gamma;

matB(1, 1) = matB(1, 1) + gamma*s(1);
matB(2, 2) = matB(2, 2) + gamma*s(2);
matB(3, 3) = matB(3, 3) + gamma*s(3);

disp('matA: ')
for i=1:3
    disp(num2str(matA(i, :)))
end

disp('matB: ')
for i=1:3
    disp(num2str(matB(i, :)))
end

wCur = single(csvread('D:/data/out/wCur.csv'));
wRef = single(csvread('D:/data/out/wRef.csv'));

wC = single([0 0 0]);
wR = single([0 0 0]);
for i=1:7
    wC(1) = wC(1) + sum(wCur(:, (i - 1)*3 + 1));
    wC(2) = wC(2) + sum(wCur(:, (i - 1)*3 + 2));
    wC(3) = wC(3) + sum(wCur(:, (i - 1)*3 + 3));
    
    wR(1) = wR(1) + sum(wRef(:, (i - 1)*3 + 1));
    wR(2) = wR(2) + sum(wRef(:, (i - 1)*3 + 2));
    wR(3) = wR(3) + sum(wRef(:, (i - 1)*3 + 3));
end

disp(['wCur: ', num2str(wC)])
disp(['wRef: ', num2str(wR)])