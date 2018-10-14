function m = pose2mat(q, t)

m = zeros(4, 4);

if(numel(q) ~= 4)
    return;
end
if(numel(t) ~= 3)
    return;
end

m(1:3, 1:3) = quat2mat(q);

m(1, 4) = t(1);
m(2, 4) = t(2);
m(3, 4) = t(3);
m(4, 4) = 1;

end