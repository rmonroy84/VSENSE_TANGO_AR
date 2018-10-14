function ptP = project(pt, f, c, dim, zeroIn)

ptP(1) = floor(pt(1)*f(1) + c(1) + 0.5);
ptP(2) = floor(pt(2)*f(2) + c(2) + 0.5);

if(size(dim, 1) ~= 0)
    ptP(1) = int16(max(0, min(ptP(1), dim(1))));
    ptP(2) = int16(max(0, min(ptP(2), dim(2))));
end

if zeroIn == 0
   ptP = ptP + [1 1]; 
end

ptP = uint16(ptP);

end