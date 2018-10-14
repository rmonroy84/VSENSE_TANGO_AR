function [pc c intrinsics extrinsics] = readPointCloud(file)
disp('Reading point cloud...');

fid = fopen(file, 'r');
intrinsics.width = fread(fid, 1, 'uint32');
intrinsics.height = fread(fid, 1, 'uint32');
intrinsics.f = fread(fid, 2, 'double');
intrinsics.c = fread(fid, 2, 'double');
intrinsics.d = fread(fid, 5, 'double');
nbrPts = fread(fid, 1, 'uint32');
timestamp = fread(fid, 1, 'double');
extrinsics.t = fread(fid, 3, 'double');
extrinsics.o = fread(fid, 4, 'double');
extrinsics.acc = fread(fid, 1, 'float');

pc = zeros(nbrPts, 4);

curIdx = 1;
for j=1:nbrPts
    pc(j, 1:3) = fread(fid, 3, 'float');    
    c(j) = fread(fid, 1, 'float');
end
fclose(fid);

disp('Reading point cloud... Finished!');

end