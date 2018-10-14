function ptD = undistort(pt, dCoeff)

x2 = pt(1)*pt(1);
y2 = pt(2)*pt(2);
r2 = x2 + y2;

if(dCoeff(4) ~= 0 & dCoeff(5) ~= 0)
    k1 = dCoeff(1);
    k2 = dCoeff(2);
    p1 = dCoeff(3);
    p2 = dCoeff(4);
    k3 = dCoeff(5);

    xy2 = 2*pt(1)*pt(2);
    kr = 1 + ((k3*r2 + k2)*r2 + k1)*r2;

    ptD(1) = pt(1)*kr + p1*xy2 + p2*(r2 + 2*x2);
    ptD(2) = pt(2)*kr + p1*(r2 + 2*y2) + p2*xy2; 
else
    k1 = dCoeff(1);
    k2 = dCoeff(2);
    k3 = dCoeff(3);
    
    kr = 1 + ((k3*r2 + k2)*r2 + k1)*r2;
    
    ptD(1) = pt(1)*kr;
    ptD(2) = pt(2)*kr;
end

end