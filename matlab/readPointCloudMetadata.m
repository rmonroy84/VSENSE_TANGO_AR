function meta = readPointCloudMetadata(file)
fid = fopen(file, 'r');

while ~feof(fid)
    line = fgets(fid);
    lineList = strsplit(line);
    if(numel(lineList) > 1)
        if(strcmp(lineList(1), 'Points:'))
            meta.points = str2num(char(lineList(2)));            
        elseif(strcmp(lineList(1), 'Timestamp:'))
           meta.timestamp = str2double(char(lineList(2)));
        elseif(strcmp(lineList(2), 'Accuracy:'))
            meta.pose.acc = str2double(char(lineList(3)));            
        elseif(strcmp(lineList(2), 'Confidence:'))
            meta.pose.conf = str2double(char(lineList(3)));
        elseif(strcmp(lineList(2), 'Orientation:'))            
            meta.pose.q(1) = str2double(lineList(3));
            meta.pose.q(2) = str2double(lineList(4));
            meta.pose.q(3) = str2double(lineList(5));
            meta.pose.q(4) = str2double(lineList(6));                        
        elseif(strcmp(lineList(2), 'Translation:'))            
            meta.pose.t(1) = str2double(lineList(3));
            meta.pose.t(2) = str2double(lineList(4));
            meta.pose.t(3) = str2double(lineList(5));
        end    
    end
end
fclose(fid);
end