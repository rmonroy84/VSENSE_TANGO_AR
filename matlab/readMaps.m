clear
clc

folder = 'D:/data/out/';
octreeFile = 'colorOT.bin';
emFile = 'colorEM.bin';
depthFile = 'depth.bin';

fid = fopen([folder, octreeFile], 'r');
width = fread(fid, 1, 'uint32');
height = fread(fid, 1, 'uint32');
r = fread(fid, [width height], 'float')';
g = fread(fid, [width height], 'float')';
b = fread(fid, [width height], 'float')';
img(:, :, 1) = r;
img(:, :, 2) = g;
img(:, :, 3) = b;
imwrite(uint8(img*255), [folder, 'colorOT.png']);
fclose(fid);

fid = fopen([folder, emFile], 'r');
width = fread(fid, 1, 'uint32');
height = fread(fid, 1, 'uint32');
r = fread(fid, [width height], 'float')';
g = fread(fid, [width height], 'float')';
b = fread(fid, [width height], 'float')';
img(:, :, 1) = r;
img(:, :, 2) = g;
img(:, :, 3) = b;
imwrite(uint8(img*255), [folder, 'colorEM.png']);
fclose(fid);

fid = fopen([folder, depthFile], 'r');
width = fread(fid, 1, 'uint32');
height = fread(fid, 1, 'uint32');
img = fread(fid, [width height], 'float')';
minDepth = 0;%min(img(:));
maxDepth = 1.0;%max(img(:));
img(img > maxDepth) = maxDepth;
img = 1.0 - (img - minDepth)/(maxDepth - minDepth);
imwrite(uint8(img*255), [folder, 'depth.png']);
fclose(fid);