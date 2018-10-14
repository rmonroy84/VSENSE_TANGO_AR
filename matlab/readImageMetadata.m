function meta = readImageMetadata(file)
fid = fopen(file, 'r');

while ~feof(fid)
    line = fgets(fid);
    lineList = strsplit(line);
    if(numel(lineList) > 1)
        if(strcmp(lineList(1), 'Width:'))
            meta.width = str2num(char(lineList(2)));            
        elseif(strcmp(lineList(1), 'Height:'))            
            meta.height = str2num(char(lineList(2)));            
        elseif(strcmp(lineList(1), 'Exposure:'))
           meta.exposure = str2double(char(lineList(2)));
        elseif(strcmp(lineList(1), 'Timestamp:'))
           meta.timestamp = str2double(char(lineList(2)));
        elseif(strcmp(lineList(2), 'Width:'))
            meta.intrinsics.width = str2num(char(lineList(3)));            
        elseif(strcmp(lineList(2), 'Height:'))
            meta.intrinsics.height = str2num(char(lineList(3)));            
        elseif(strcmp(lineList(2), 'cx:'))
            meta.intrinsics.cx = str2double(char(lineList(3)));
        elseif(strcmp(lineList(2), 'cy:'))
            meta.intrinsics.cy = str2double(char(lineList(3)));
        elseif(strcmp(lineList(2), 'fx:'))
            meta.intrinsics.fx = str2double(char(lineList(3)));
        elseif(strcmp(lineList(2), 'fy:'))
            meta.intrinsics.fy = str2double(char(lineList(3)));
        elseif(strcmp(lineList(2), 'Distortion:'))            
            meta.intrinsics.k1 = str2double(lineList(3));
            meta.intrinsics.k2 = str2double(lineList(4));
            meta.intrinsics.k3 = str2double(lineList(5));
            meta.intrinsics.k4 = str2double(lineList(6));
        elseif(strcmp(lineList(2), 'Accuracy:'))
            meta.pose.acc = str2double(char(lineList(3)));            
        elseif(strcmp(lineList(2), 'Confidence:'))
            meta.pose.conf = str2double(char(lineList(3)));
        elseif(strcmp(lineList(2), 'DC_Accuracy:'))
            meta.depPose.acc = str2double(char(lineList(3)));
        elseif(strcmp(lineList(2), 'DC_Confidence:'))
            meta.depPose.conf = str2double(char(lineList(3)));
        elseif(strcmp(lineList(2), 'Orientation:'))            
            meta.pose.q(1) = str2double(lineList(3));
            meta.pose.q(2) = str2double(lineList(4));
            meta.pose.q(3) = str2double(lineList(5));
            meta.pose.q(4) = str2double(lineList(6));                        
        elseif(strcmp(lineList(2), 'Translation:'))            
            meta.pose.t(1) = str2double(lineList(3));
            meta.pose.t(2) = str2double(lineList(4));
            meta.pose.t(3) = str2double(lineList(5));
        elseif(strcmp(lineList(2), 'DC_Orientation:'))            
            meta.depPose.q(1) = str2double(lineList(3));
            meta.depPose.q(2) = str2double(lineList(4));
            meta.depPose.q(3) = str2double(lineList(5));
            meta.depPose.q(4) = str2double(lineList(6));
        elseif(strcmp(lineList(2), 'DC_Translation:'))            
            meta.depPose.t(1) = str2double(lineList(3));
            meta.depPose.t(2) = str2double(lineList(4));
            meta.depPose.t(3) = str2double(lineList(5));
        end    
    end
end
fclose(fid);
end