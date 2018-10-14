function m = quat2mat(q)

m = zeros(3, 3);

if(numel(q) ~= 4)
    return;
end

x = q(1);
y = q(2);
z = q(3);
w = q(4);

tx  = 2*x;
ty  = 2*y;
tz  = 2*z;
twx = tx*w;
twy = ty*w;
twz = tz*w;
txx = tx*x;
txy = ty*x;
txz = tz*x;
tyy = ty*y;
tyz = tz*y;
tzz = tz*z;

m(1, 1) = 1 - (tyy + tzz);
m(1, 2) = txy - twz;
m(1, 3) = txz + twy;
m(2, 1) = txy + twz;
m(2, 2) = 1 - (txx + tzz);
m(2, 3) = tyz - twx;
m(3, 1) = txz - twy;
m(3, 2) = tyz + twx;
m(3, 3) = 1 - (txx + tyy);
        
end