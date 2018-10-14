function [color depth dist] = getKnownDepth(dMap, iMap, pos, dir)
    continueFlag = true;
    curPos = pos;
    
    dist = 0;    
    color = [0 0 0];
    depth = 0;
    
    while(true)
        curPos = curPos + dir;
        
        if(curPos(1) < 1)
            break;
        elseif(curPos(1) > size(dMap, 2))
            break;
        elseif(curPos(2) < 1)
            break;
        elseif(curPos(2) > size(dMap, 1))
            break;
        end
        
        if(dMap(curPos(2), curPos(1)) > 0)
           color(1:3) = iMap(curPos(2), curPos(1), :);
           depth = dMap(curPos(2), curPos(1));           
           dist = norm(curPos - pos);           
           break;
        end
    end
end