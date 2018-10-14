function m = frustum(left, right, bottom, top, nearVal, farVal)
m = zeros(4, 4);
m(1, 1) = 2*nearVal/(right - left);
m(2, 2) = 2*nearVal/(top - bottom);
m(1, 3) = (right + left)/(right - left);
m(2, 3) = (top + bottom) / (top - bottom);
m(3, 3) = -(farVal + nearVal)/(farVal - nearVal);
m(4, 3) = -1;
m(3, 4) = -(2*farVal*nearVal)/(farVal - nearVal);
end