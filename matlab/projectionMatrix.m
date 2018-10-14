function m = projectionMatrix(width, height, fx, fy, cx, cy, near, far)

xscale = near/fx;
yscale = near/fy;

xoffset = (cx - (width/2.0))*xscale;
yoffset = -(cy - (height/2.0))*yscale;

m = frustum(xscale*-width/2.0 - xoffset, ...
        xscale * width/2.0 - xoffset,  ...
        yscale * -height/2.0 - yoffset,...
        yscale * height/2.0 - yoffset, near, far);

end