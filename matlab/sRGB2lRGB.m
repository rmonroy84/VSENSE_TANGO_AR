function lRGB = sRGB2lRGB(sRGB)

lRGB = sRGB;
for j=1:3
   if(lRGB(j) <= 0.04045)
       lRGB(j) = lRGB(j)/12.92;
   else
       lRGB(j) = ((lRGB(j) + 0.055)/1.055)^2.4;
   end
end

end