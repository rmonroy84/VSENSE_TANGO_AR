function [img intrinsics extrinsics] = readColorImage(file)

fid = fopen(file);

width = fread(fid, 1, 'uint32');
height = fread(fid, 1, 'uint32');
exposure = fread(fid, 1, 'int64');
timestamp = fread(fid, 1, 'double');
intrinsics.f = fread(fid, 2, 'double');
intrinsics.c = fread(fid, 2, 'double');
intrinsics.d = fread(fid, 5, 'double');
extrinsics.t = fread(fid, 3, 'double');
extrinsics.o = fread(fid, 4, 'double');
extrinsics.accuracy = fread(fid, 1, 'float');
Y = fread(fid, [width height], 'uint8')';
C = fread(fid, [width height/2], 'uint8')';
fclose(fid);

for col = 1:size(C, 2)/2
    Cr(:, col) = C(:, col*2 - 1);
    Cb(:, col) = C(:, col*2);
end

Cr = imresize(Cr, [height width]);
Cb = imresize(Cb, [height width]);

rTmp = Y + (1.370705*(Cr - 128));
gTmp = Y - (0.698001*(Cr - 128)) - (0.337633*(Cb - 128));
bTmp = Y + (1.732446*(Cb - 128));

r = max(min(rTmp, 255), 0);
g = max(min(gTmp, 255), 0);
b = max(min(bTmp, 255), 0);

img(:, :, 1) = uint8(r);
img(:, :, 2) = uint8(g);
img(:, :, 3) = uint8(b);

end