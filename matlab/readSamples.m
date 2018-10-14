clc
clear

samples = csvread('D:/data/out/samples.csv');
samplesRef = samples(:, 1:3);
samplesCur = samples(:, 4:6);
matA = csvread('D:/data/out/matA.csv');
matB = csvread('D:/data/out/matB.csv');
matCor = csvread('D:/data/out/matCorr.csv');

nbrSamples = size(samples, 1);

samplesCor = samplesCur*matCor;

diffNoCor = samplesRef - samplesCur;
diffCor = samplesRef - samplesCor;

sumErrorNoCor = 0;
sumErrorCor = 0;
for n = 1:nbrSamples
    sumErrorNoCor = sumErrorNoCor + sqrt(diffNoCor(n, :)*diffNoCor(n, :)');
    sumErrorCor = sumErrorCor + sqrt(diffCor(n, :)*diffCor(n, :)');
end

sumErrorNoCor = sumErrorNoCor / nbrSamples;
sumErrorCor = sumErrorCor / nbrSamples;

disp(['No correction: ', num2str(sumErrorNoCor)])
disp(['Correction: ', num2str(sumErrorCor)])

ws = sum(samplesCur)./sum(samplesRef);
samplesWCor(:, 1) = ws(1)*samplesCur(:, 1);
samplesWCor(:, 2) = ws(2)*samplesCur(:, 2);
samplesWCor(:, 3) = ws(3)*samplesCur(:, 3);

diffWCor = samplesRef - samplesWCor;

sumWErrorCor = 0;
for n = 1:nbrSamples    
    sumWErrorCor = sumWErrorCor + sqrt(diffWCor(n, :)*diffWCor(n, :)');
end
sumWErrorCor = sumWErrorCor / nbrSamples;

disp(['Correction (W): ', num2str(sumWErrorCor)])



newCor(:, 1) = lsqlin(samplesCur, samplesRef(:, 1));
newCor(:, 2) = lsqlin(samplesCur, samplesRef(:, 2));
newCor(:, 3) = lsqlin(samplesCur, samplesRef(:, 3));

newSamplesCor(:, 1) = samplesCur*newCor(:, 1);
newSamplesCor(:, 2) = samplesCur*newCor(:, 2);
newSamplesCor(:, 3) = samplesCur*newCor(:, 3);

newDiff = samplesRef - newSamplesCor;

newError = 0;
for n = 1:nbrSamples   
    newError = newError + sqrt(newDiff(n, :)*newDiff(n, :)');
end
newError = newError/nbrSamples;

disp(['Correction (New): ', num2str(newError)])

