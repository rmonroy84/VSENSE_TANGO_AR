function sRGB = linRGB2sRGB(rgb)
 	sRGB = rgb;

	for i=1:3
		if (sRGB(i) <= 0.0031308)
			sRGB(i) = sRGB(i)*12.92;
		else
			sRGB(i) = 1.055*(sRGB(i)^(1/2.4)) - 0.055;
        end
    end
end