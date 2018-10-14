function [minVal maxVal] = createDepthMap(nbrFile)

clc
close all

imFile = ['D:/data/27/PointCloud', num2str(nbrFile), '.im'];
pcFile = ['D:/data/27/PointCloud', num2str(nbrFile), '.pc'];

minConf = 1.0;

[img intrinsics_i extrinsics_i] = readColorImage(imFile);
[pc c intrinsics_p extrinsics_p] = readPointCloud(pcFile);

poseIM = pose2mat(extrinsics_i.o, extrinsics_i.t);
posePC = pose2mat(extrinsics_p.o, extrinsics_p.t);

img = double(img)/255;

dim_depth = uint16([intrinsics_p.width intrinsics_p.height]);
dim_color = uint16([1920 1080]);

dMap = -ones(dim_depth(2), dim_depth(1), 1);
iMap = zeros(dim_depth(2), dim_depth(1), 3);

load('ptMap.mat');

idx = 1;
for j=1:size(pc, 1)                
    pt = [pc(j, 1:3) 1];
    
    x = pt(1)/pt(3);
    y = pt(2)/pt(3);
    ptDepth = undistort([x, y], intrinsics_p.d);
    ptDepth = project(ptDepth, intrinsics_p.f, intrinsics_p.c, dim_depth, 0);
    ptDepth = dim_depth - ptDepth + uint16([1 1]);  
    
    ptTrans = poseIM*pt';

    ptColor = undistort([ptTrans(1)/ptTrans(3), ptTrans(2)/ptTrans(3)], intrinsics_i.d);
    ptColor = project(ptColor, intrinsics_i.f, intrinsics_i.c, [], 0);
    	
    %iMap(ptDepth(2), ptDepth(1), 1:3) = [1 0 0];
    
    if (ptColor(1) <= 0)
        continue;
    end
        
    if (ptColor(1) > dim_color(1))
        continue;	
    end

    if (ptColor(2) <= 0)
        continue;
    end
        
    if (ptColor(2) > dim_color(2))
        continue;	
    end  

    iMap(ptDepth(2), ptDepth(1), 1:3) = img(ptColor(2), ptColor(1), :);
    if(c(j) >= minConf)        
        dMap(ptDepth(2), ptDepth(1), 1) = ptTrans(3);                
        
        pts(idx, 1:3) = ptTrans(1:3);
        pts(idx, 4:6) = sRGB2lRGB(img(ptColor(2), ptColor(1), :)); %Linearize assume sRGB	               
        
        idx = idx + 1;    
    end	
end

imshow(iMap);
figure;
imshow(dMap, [])

for row = 1:dim_depth(2)
    for col = 1:dim_depth(1)
        if(dMap(row, col) > 0)
            continue;
        end              
        
        x = ptMap(row, col, 1);
        y = ptMap(row, col, 2);
        
        if(x == 0 & y == 0)
            continue;
        end
        
        pt = [x y 1 1];
        ptTrans = poseIM*pt';
        
        ptColor = undistort([ptTrans(1)/ptTrans(3), ptTrans(2)/ptTrans(3)], intrinsics_i.d);
        ptColor = project(ptColor, intrinsics_i.f, intrinsics_i.c, [], 0);  
        
        if (ptColor(1) <= 0)
            continue;
        elseif (ptColor(1) > dim_color(1))
            continue;	
        elseif (ptColor(2) <= 0)
            continue;
        elseif (ptColor(2) > dim_color(2))
            continue;	
        end 
        
        pos = double([col row]);
        
        c = zeros(8, 3);
        [c(1, :) d(1) dt(1)] = getKnownDepth(dMap, iMap, pos, [-1 -1]); 
        [c(2, :) d(2) dt(2)] = getKnownDepth(dMap, iMap, pos, [-1  0]);
        [c(3, :) d(3) dt(3)] = getKnownDepth(dMap, iMap, pos, [-1  1]); 
        [c(4, :) d(4) dt(4)] = getKnownDepth(dMap, iMap, pos, [ 0 -1]);
        [c(5, :) d(5) dt(5)] = getKnownDepth(dMap, iMap, pos, [ 0  1]); 
        [c(6, :) d(6) dt(6)] = getKnownDepth(dMap, iMap, pos, [ 1 -1]);
        [c(7, :) d(7) dt(7)] = getKnownDepth(dMap, iMap, pos, [ 1  0]);                                        
        [c(8, :) d(8) dt(8)] = getKnownDepth(dMap, iMap, pos, [ 1  1]); 
    
        dtWeight = (dt ~= 0).*(double(dim_depth(1)) - dt)/double(dim_depth(1));
        dSum = sum(d.*dtWeight);
    
        dMap(row, col, 1) = dSum/sum(dtWeight);
        iMap(row, col, 1:3) = img(ptColor(2), ptColor(1), :);
    end
end

figure;
imshow(dMap, [])
figure;
imshow(iMap);

end