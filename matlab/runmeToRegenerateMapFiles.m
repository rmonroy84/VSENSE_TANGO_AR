% Random spherical coordinates generator. If you generate a new one, you'll have to re-calculate all SH coefficients for the meshes.

fid = fopen('random.bin', 'wb');
nbrSamples = 100000;
fwrite(fid, nbrSamples, 'uint32');
for j=1:nbrSamples    
    theta = 2 * acos(sqrt(1 - rand));
    phi = 2*pi*rand;
    
    fwrite(fid, theta, 'float');
    fwrite(fid, phi, 'float');
end
fclose(fid);

    
% Code to generate the projection mapping considering intrinsics and lens distortions. Make sure the data below matches those from your phone.
% This code is highly inefficient and finds the matches through brute force.
    
dCoeff_depth = [-0.12967200000000001, -1.3787799999999999, -0.00281168, 0.00086066599999999995, 2.9144299999999999];
f_depth = [215.541 215.541];
c_depth = [111.928 88.1233];
dim_depth = uint16([224 172]);

%mask = imread('mask.png');

ptMap = zeros(dim_depth(2), dim_depth(1), 3);

step = 0.00025;

curPer = int8(-1);
for j=-0.57:step:0.57
	for k=-0.67:step:0.67
		ptDepth = undistort([j, k], dCoeff_depth);
		ptDepth = project(ptDepth, f_depth, c_depth, [], 0);
		ptDepth = dim_depth - ptDepth + uint16([1 1]);
			   
		if(ptDepth(1) < 1)
			continue;
		elseif(ptDepth(1) > dim_depth(1))
			continue;
		elseif(ptDepth(2) < 1)
			continue;
		elseif(ptDepth(2) > dim_depth(2))
			continue;
		%elseif(mask(ptDepth(2), ptDepth(1), 1) == 0)
		%    continue     
		end

		ptMap(ptDepth(2), ptDepth(1), 1) = ptMap(ptDepth(2), ptDepth(1), 1) + j;
		ptMap(ptDepth(2), ptDepth(1), 2) = ptMap(ptDepth(2), ptDepth(1), 2) + k;
		ptMap(ptDepth(2), ptDepth(1), 3) = ptMap(ptDepth(2), ptDepth(1), 3) + 1;
	end

	per = int8((j - 0.57)*100/(2*.57));
	if per ~= curPer
		curPer = per;
		disp(per)
	end
end

for j=1:dim_depth(2)
	for k=1:dim_depth(1)
		if ptMap(j, k, 3) > 0
			ptMap(j, k, 1:2) = ptMap(j, k, 1:2)/ptMap(j, k, 3); 
		end
	end
end

save('ptMap.mat', 'ptMap');

fid = fopen('ptMap.bin', 'wb');
for j=1:172
   for k=1:224
	  fwrite(fid, ptMap(j, k, 1), 'float');
	  fwrite(fid, ptMap(j, k, 2), 'float');
   end
end
fclose(fid);